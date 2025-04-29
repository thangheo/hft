"""
Resource Monitor Module

This module provides monitoring of system resources including CPU, memory, disk, and network.

Classes:
    ResourceMonitor: Main class for monitoring system resources
"""

import os
import psutil
import time
from threading import Thread
from collections import deque


class ResourceMonitor:
    """
    Monitors system resources including CPU, memory, disk, and network.
    
    Attributes:
        monitoring_interval (float): Interval between resource checks in seconds
        history_size (int): Number of historical measurements to keep
        thresholds (dict): Thresholds for resource usage alerts
        metrics (dict): Dictionary of resource metrics
        running (bool): Flag indicating if the monitor is running
    """
    
    def __init__(self, monitoring_interval=5.0, history_size=720, thresholds=None):
        """
        Initialize the resource monitor.
        
        Args:
            monitoring_interval (float): Interval between resource checks in seconds
            history_size (int): Number of historical measurements to keep
            thresholds (dict): Thresholds for resource usage alerts
        """
        self.monitoring_interval = monitoring_interval
        self.history_size = history_size
        self.thresholds = thresholds or {
            'cpu_percent': 80.0,
            'memory_percent': 85.0,
            'disk_percent': 90.0,
            'network_mbps': 800.0
        }
        
        self.metrics = {
            'cpu': deque(maxlen=history_size),
            'memory': deque(maxlen=history_size),
            'disk': deque(maxlen=history_size),
            'network': deque(maxlen=history_size)
        }
        
        self.running = False
        self.monitor_thread = None
        self.last_network_io = None
    
    def start_monitoring(self):
        """Start the resource monitoring thread."""
        if self.running:
            return
        
        self.running = True
        self.monitor_thread = Thread(target=self._monitoring_loop, daemon=True)
        self.monitor_thread.start()
    
    def stop_monitoring(self):
        """Stop the resource monitoring thread."""
        self.running = False
        if self.monitor_thread:
            self.monitor_thread.join(timeout=self.monitoring_interval*2)
    
    def _monitoring_loop(self):
        """Main monitoring loop that collects resource metrics."""
        while self.running:
            try:
                timestamp = time.time()
                
                # CPU usage
                cpu_percent = psutil.cpu_percent(interval=None)
                self.metrics['cpu'].append({
                    'timestamp': timestamp,
                    'percent': cpu_percent,
                    'count': psutil.cpu_count(),
                    'per_cpu': psutil.cpu_percent(interval=None, percpu=True)
                })
                
                # Memory usage
                memory = psutil.virtual_memory()
                self.metrics['memory'].append({
                    'timestamp': timestamp,
                    'total': memory.total,
                    'available': memory.available,
                    'used': memory.used,
                    'percent': memory.percent
                })
                
                # Disk usage
                disk = psutil.disk_usage('/')
                self.metrics['disk'].append({
                    'timestamp': timestamp,
                    'total': disk.total,
                    'used': disk.used,
                    'free': disk.free,
                    'percent': disk.percent
                })
                
                # Network usage
                network_io = psutil.net_io_counters()
                if self.last_network_io:
                    # Calculate rate in bytes per second
                    recv_rate = (network_io.bytes_recv - self.last_network_io.bytes_recv) / self.monitoring_interval
                    sent_rate = (network_io.bytes_sent - self.last_network_io.bytes_sent) / self.monitoring_interval
                    
                    # Convert to Mbps
                    recv_mbps = recv_rate * 8 / 1_000_000
                    sent_mbps = sent_rate * 8 / 1_000_000
                    
                    self.metrics['network'].append({
                        'timestamp': timestamp,
                        'bytes_sent': network_io.bytes_sent,
                        'bytes_recv': network_io.bytes_recv,
                        'sent_mbps': sent_mbps,
                        'recv_mbps': recv_mbps,
                        'total_mbps': recv_mbps + sent_mbps
                    })
                
                self.last_network_io = network_io
                
            except Exception as e:
                print(f"Error collecting resource metrics: {e}")
            
            # Sleep until next collection
            time.sleep(self.monitoring_interval)
    
    def get_latest_metrics(self):
        """
        Get the latest resource metrics.
        
        Returns:
            dict: Latest resource metrics
        """
        result = {
            'timestamp': time.time()
        }
        
        for resource_type, measurements in self.metrics.items():
            if measurements:
                result[resource_type] = measurements[-1]
        
        return result
    
    def get_historical_metrics(self, resource_type, time_range=None):
        """
        Get historical metrics for a resource type.
        
        Args:
            resource_type (str): Type of resource ('cpu', 'memory', 'disk', 'network')
            time_range (tuple): Optional time range (start, end) in seconds
        
        Returns:
            list: Historical metrics for the resource type
        """
        if resource_type not in self.metrics:
            return []
        
        measurements = list(self.metrics[resource_type])
        
        # Filter by time range if provided
        if time_range:
            start_time, end_time = time_range
            measurements = [m for m in measurements 
                            if start_time <= m['timestamp'] <= end_time]
        
        return measurements
    
    def check_thresholds(self):
        """
        Check if any resource metrics exceed defined thresholds.
        
        Returns:
            dict: Resource threshold violations
        """
        violations = {}
        latest = self.get_latest_metrics()
        
        # Check CPU threshold
        if 'cpu' in latest and latest['cpu']['percent'] > self.thresholds['cpu_percent']:
            violations['cpu'] = {
                'threshold': self.thresholds['cpu_percent'],
                'value': latest['cpu']['percent'],
                'message': f"CPU usage ({latest['cpu']['percent']}%) exceeds threshold ({self.thresholds['cpu_percent']}%)"
            }
        
        # Check memory threshold
        if 'memory' in latest and latest['memory']['percent'] > self.thresholds['memory_percent']:
            violations['memory'] = {
                'threshold': self.thresholds['memory_percent'],
                'value': latest['memory']['percent'],
                'message': f"Memory usage ({latest['memory']['percent']}%) exceeds threshold ({self.thresholds['memory_percent']}%)"
            }
        
        # Check disk threshold
        if 'disk' in latest and latest['disk']['percent'] > self.thresholds['disk_percent']:
            violations['disk'] = {
                'threshold': self.thresholds['disk_percent'],
                'value': latest['disk']['percent'],
                'message': f"Disk usage ({latest['disk']['percent']}%) exceeds threshold ({self.thresholds['disk_percent']}%)"
            }
        
        # Check network threshold
        if 'network' in latest and latest['network']['total_mbps'] > self.thresholds['network_mbps']:
            violations['network'] = {
                'threshold': self.thresholds['network_mbps'],
                'value': latest['network']['total_mbps'],
                'message': f"Network usage ({latest['network']['total_mbps']} Mbps) exceeds threshold ({self.thresholds['network_mbps']} Mbps)"
            }
        
        return violations
