"""
Log Collector Module

This module provides centralized log collection, filtering, and forwarding capabilities.

Classes:
    LogCollector: Main class for log collection and management
"""

import os
import time
import logging
import logging.handlers
import queue
from threading import Thread, Lock


class LogCollector:
    """
    Collects, filters, and forwards logs from multiple sources.
    
    Attributes:
        log_queue (Queue): Queue for collecting logs from multiple sources
        log_filters (list): List of log filtering functions
        log_handlers (dict): Dictionary of log handlers for different destinations
        running (bool): Flag indicating if the collector is running
    """
    
    def __init__(self, config=None):
        """
        Initialize the log collector.
        
        Args:
            config (dict): Optional configuration for the log collector
        """
        self.log_queue = queue.Queue()
        self.log_filters = []
        self.log_handlers = {}
        self.running = False
        self.processor_thread = None
        self.log_lock = Lock()
        
        # Configure default handlers
        self.configure_default_handlers()
        
        if config:
            self.configure(config)
    
    def configure(self, config):
        """
        Configure the log collector with the provided configuration.
        
        Args:
            config (dict): Configuration for the log collector
        """
        # TODO: Implement configuration loading
        pass
    
    def configure_default_handlers(self):
        """Configure default log handlers."""
        # Console handler
        console_handler = logging.StreamHandler()
        console_handler.setLevel(logging.INFO)
        formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
        console_handler.setFormatter(formatter)
        
        self.log_handlers['console'] = console_handler
        
        # File handler (rotating)
        try:
            os.makedirs('logs', exist_ok=True)
            file_handler = logging.handlers.RotatingFileHandler(
                'logs/system.log', maxBytes=10*1024*1024, backupCount=5)
            file_handler.setLevel(logging.DEBUG)
            file_handler.setFormatter(formatter)
            
            self.log_handlers['file'] = file_handler
        except Exception as e:
            print(f"Error configuring file handler: {e}")
    
    def add_log_filter(self, filter_func):
        """
        Add a log filtering function.
        
        Args:
            filter_func (callable): Function that takes a log record and returns
                                   True if it should be processed, False otherwise
        """
        self.log_filters.append(filter_func)
    
    def add_log_handler(self, name, handler):
        """
        Add a log handler.
        
        Args:
            name (str): Name of the handler
            handler (logging.Handler): Log handler
        """
        with self.log_lock:
            self.log_handlers[name] = handler
    
    def remove_log_handler(self, name):
        """
        Remove a log handler.
        
        Args:
            name (str): Name of the handler to remove
        
        Returns:
            bool: True if handler was removed, False otherwise
        """
        with self.log_lock:
            if name in self.log_handlers:
                del self.log_handlers[name]
                return True
        return False
    
    def start_collection(self):
        """Start the log collection and processing thread."""
        if self.running:
            return
        
        self.running = True
        self.processor_thread = Thread(target=self._processing_loop, daemon=True)
        self.processor_thread.start()
    
    def stop_collection(self):
        """Stop the log collection and processing thread."""
        self.running = False
        if self.processor_thread:
            self.processor_thread.join(timeout=5.0)
    
    def _processing_loop(self):
        """Main log processing loop."""
        while self.running:
            try:
                # Get log record from queue (with timeout to check self.running periodically)
                try:
                    log_record = self.log_queue.get(timeout=1.0)
                except queue.Empty:
                    continue
                
                # Apply filters
                should_process = True
                for filter_func in self.log_filters:
                    if not filter_func(log_record):
                        should_process = False
                        break
                
                if not should_process:
                    self.log_queue.task_done()
                    continue
                
                # Process log record through handlers
                with self.log_lock:
                    for handler in self.log_handlers.values():
                        try:
                            if log_record.levelno >= handler.level:
                                handler.emit(log_record)
                        except Exception as e:
                            print(f"Error processing log record: {e}")
                
                self.log_queue.task_done()
                
            except Exception as e:
                print(f"Error in log processing loop: {e}")
    
    def get_logger(self, name):
        """
        Get a logger that sends logs to this collector.
        
        Args:
            name (str): Logger name
        
        Returns:
            logging.Logger: Configured logger
        """
        logger = logging.getLogger(name)
        
        # Remove existing handlers if any
        for handler in logger.handlers[:]:
            logger.removeHandler(handler)
        
        # Add queue handler
        queue_handler = logging.handlers.QueueHandler(self.log_queue)
        logger.addHandler(queue_handler)
        
        # Set level to DEBUG to capture all logs (filtering happens in the collector)
        logger.setLevel(logging.DEBUG)
        
        return logger
    
    def get_recent_logs(self, count=100, level=logging.INFO, search_text=None):
        """
        Get recent logs from the file handler.
        
        Args:
            count (int): Number of log entries to retrieve
            level (int): Minimum log level to include
            search_text (str): Optional text to search for in log messages
        
        Returns:
            list: Recent log entries
        """
        # TODO: Implement log retrieval from storage
        return []
