"""
Metrics Processor Module

This module processes various system metrics, performs time series analysis,
and manages historical data storage.

Classes:
    MetricsProcessor: Main class for metrics processing
"""

import time
import json
import numpy as np
from datetime import datetime, timedelta
from collections import defaultdict, deque


class MetricsProcessor:
    """
    Processes system metrics and performs time series analysis.
    
    Attributes:
        metrics_store (dict): In-memory store for recent metrics
        retention_period (int): Retention period in seconds
        database_connector: Connector to external time-series database
    """
    
    def __init__(self, retention_period=86400, database_connector=None):
        """
        Initialize the metrics processor.
        
        Args:
            retention_period (int): Retention period in seconds (default: 24 hours)
            database_connector: Optional connector to external time-series database
        """
        self.metrics_store = defaultdict(lambda: deque(maxlen=10000))
        self.retention_period = retention_period
        self.database_connector = database_connector
    
    def store_metric(self, metric_name, value, tags=None, timestamp=None):
        """
        Store a metric in memory and optionally in the database.
        
        Args:
            metric_name (str): Name of the metric
            value (float): Value of the metric
            tags (dict): Optional tags/dimensions for the metric
            timestamp (float): Optional timestamp (defaults to current time)
        
        Returns:
            bool: True if stored successfully, False otherwise
        """
        timestamp = timestamp or time.time()
        tags = tags or {}
        
        # Create a metric point
        point = {
            'timestamp': timestamp,
            'value': value,
            'tags': tags
        }
        
        # Store in memory
        self.metrics_store[metric_name].append(point)
        
        # Store in database if connector is available
        if self.database_connector:
            try:
                self.database_connector.write_point(metric_name, value, tags, timestamp)
            except Exception as e:
                print(f"Error writing to database: {e}")
                return False
        
        return True
    
    def store_metrics_batch(self, metrics):
        """
        Store multiple metrics at once.
        
        Args:
            metrics (list): List of metric dictionaries with keys:
                           metric_name, value, tags (optional), timestamp (optional)
        
        Returns:
            int: Number of metrics stored successfully
        """
        success_count = 0
        
        for metric in metrics:
            if self.store_metric(
                metric['metric_name'],
                metric['value'],
                metric.get('tags'),
                metric.get('timestamp')
            ):
                success_count += 1
        
        return success_count
    
    def get_metric_range(self, metric_name, start_time, end_time=None, tags=None):
        """
        Get metrics within a time range.
        
        Args:
            metric_name (str): Name of the metric
            start_time (float): Start timestamp
            end_time (float): End timestamp (defaults to current time)
            tags (dict): Optional filter by tags
        
        Returns:
            list: Metrics within the specified range
        """
        end_time = end_time or time.time()
        
        # Try to get from database first if available
        if self.database_connector:
            try:
                return self.database_connector.query_range(
                    metric_name, start_time, end_time, tags)
            except Exception as e:
                print(f"Error querying database: {e}")
        
        # Fall back to in-memory store
        if metric_name not in self.metrics_store:
            return []
        
        result = []
        for point in self.metrics_store[metric_name]:
            if start_time <= point['timestamp'] <= end_time:
                # Check tags if provided
                if tags:
                    match = True
                    for tag_key, tag_value in tags.items():
                        if point['tags'].get(tag_key) != tag_value:
                            match = False
                            break
                    
                    if not match:
                        continue
                
                result.append(point)
        
        return result
    
    def calculate_statistics(self, metric_name, start_time, end_time=None, tags=None):
        """
        Calculate statistics for a metric within a time range.
        
        Args:
            metric_name (str): Name of the metric
            start_time (float): Start timestamp
            end_time (float): End timestamp (defaults to current time)
            tags (dict): Optional filter by tags
        
        Returns:
            dict: Statistics including min, max, avg, median, etc.
        """
        metrics = self.get_metric_range(metric_name, start_time, end_time, tags)
        
        if not metrics:
            return {
                'count': 0,
                'min': None,
                'max': None,
                'avg': None,
                'median': None,
                'sum': None,
                'stddev': None
            }
        
        values = [m['value'] for m in metrics]
        
        return {
            'count': len(values),
            'min': min(values),
            'max': max(values),
            'avg': sum(values) / len(values),
            'median': np.median(values),
            'sum': sum(values),
            'stddev': np.std(values) if len(values) > 1 else 0
        }
    
    def detect_anomalies(self, metric_name, window_size=3600, threshold=2.0, tags=None):
        """
        Detect anomalies in a metric using a simple Z-score method.
        
        Args:
            metric_name (str): Name of the metric
            window_size (int): Size of the window in seconds for baseline
            threshold (float): Z-score threshold for anomaly detection
            tags (dict): Optional filter by tags
        
        Returns:
            list: Detected anomalies
        """
        end_time = time.time()
        start_time = end_time - window_size
        
        metrics = self.get_metric_range(metric_name, start_time, end_time, tags)
        
        if len(metrics) < 10:  # Need enough data points for meaningful detection
            return []
        
        values = [m['value'] for m in metrics]
        timestamps = [m['timestamp'] for m in metrics]
        
        mean = np.mean(values)
        std = np.std(values)
        
        anomalies = []
        
        if std > 0:  # Avoid division by zero
            for i, value in enumerate(values):
                z_score = abs(value - mean) / std
                
                if z_score > threshold:
                    anomalies.append({
                        'timestamp': timestamps[i],
                        'value': value,
                        'z_score': z_score,
                        'mean': mean,
                        'std': std
                    })
        
        return anomalies
    
    def cleanup_old_metrics(self):
        """
        Clean up metrics older than the retention period.
        
        Returns:
            int: Number of metrics removed
        """
        cutoff_time = time.time() - self.retention_period
        removed_count = 0
        
        for metric_name in self.metrics_store:
            original_length = len(self.metrics_store[metric_name])
            
            # Remove old metrics
            while (self.metrics_store[metric_name] and 
                   self.metrics_store[metric_name][0]['timestamp'] < cutoff_time):
                self.metrics_store[metric_name].popleft()
                removed_count += 1
        
        return removed_count
