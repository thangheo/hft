"""
Latency Tracker Module

This module provides capabilities for tracking and analyzing latency across system components.

Classes:
    LatencyTracker: Main class for tracking and analyzing latency
"""

import time
import statistics
from collections import deque


class LatencyTracker:
    """
    Tracks and analyzes latency across system components.
    
    Attributes:
        measurements (dict): Dictionary of latency measurements by category
        history_size (int): Number of historical measurements to keep
        percentiles (list): Percentiles to calculate for latency statistics
    """
    
    def __init__(self, history_size=1000, percentiles=None):
        """
        Initialize the latency tracker.
        
        Args:
            history_size (int): Number of historical measurements to keep
            percentiles (list): Percentiles to calculate (defaults to [50, 90, 95, 99])
        """
        self.measurements = {}
        self.history_size = history_size
        self.percentiles = percentiles or [50, 90, 95, 99]
    
    def start_measurement(self, category, label=None):
        """
        Start a latency measurement.
        
        Args:
            category (str): Category of the measurement (e.g., 'database', 'network')
            label (str): Optional label for the measurement
        
        Returns:
            dict: Measurement context containing start time
        """
        return {
            'category': category,
            'label': label,
            'start_time': time.perf_counter(),
        }
    
    def end_measurement(self, context):
        """
        End a latency measurement and record it.
        
        Args:
            context (dict): Measurement context from start_measurement
        
        Returns:
            float: Measured latency in milliseconds
        """
        end_time = time.perf_counter()
        latency_ms = (end_time - context['start_time']) * 1000
        
        category = context['category']
        if category not in self.measurements:
            self.measurements[category] = deque(maxlen=self.history_size)
        
        self.measurements[category].append({
            'latency_ms': latency_ms,
            'timestamp': time.time(),
            'label': context['label']
        })
        
        return latency_ms
    
    def measure(self, category, label=None):
        """
        Decorator for measuring function execution latency.
        
        Args:
            category (str): Category of the measurement
            label (str): Optional label for the measurement
        
        Returns:
            function: Decorated function
        """
        def decorator(func):
            def wrapper(*args, **kwargs):
                context = self.start_measurement(category, label)
                result = func(*args, **kwargs)
                self.end_measurement(context)
                return result
            return wrapper
        return decorator
    
    def get_statistics(self, category, time_range=None):
        """
        Get latency statistics for a category.
        
        Args:
            category (str): Category to get statistics for
            time_range (tuple): Optional time range (start, end) in seconds
        
        Returns:
            dict: Latency statistics including percentiles
        """
        if category not in self.measurements or not self.measurements[category]:
            return {
                'count': 0,
                'min': 0,
                'max': 0,
                'mean': 0,
                'percentiles': {p: 0 for p in self.percentiles}
            }
        
        measurements = self.measurements[category]
        
        # Filter by time range if provided
        if time_range:
            start_time, end_time = time_range
            measurements = [m for m in measurements 
                            if start_time <= m['timestamp'] <= end_time]
        
        if not measurements:
            return {
                'count': 0,
                'min': 0,
                'max': 0,
                'mean': 0,
                'percentiles': {p: 0 for p in self.percentiles}
            }
        
        latencies = [m['latency_ms'] for m in measurements]
        
        stats = {
            'count': len(latencies),
            'min': min(latencies),
            'max': max(latencies),
            'mean': statistics.mean(latencies) if latencies else 0,
            'percentiles': {}
        }
        
        # Calculate percentiles
        for p in self.percentiles:
            stats['percentiles'][p] = statistics.quantiles(latencies, n=100)[p-1] if latencies else 0
            
        return stats
