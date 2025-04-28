-- Create schemas
CREATE SCHEMA IF NOT EXISTS trading;
CREATE SCHEMA IF NOT EXISTS config;
CREATE SCHEMA IF NOT EXISTS metrics;

-- Create tables for strategy parameters
CREATE TABLE IF NOT EXISTS config.strategy_parameters (
    id SERIAL PRIMARY KEY,
    strategy_id VARCHAR(100) NOT NULL,
    parameter_group VARCHAR(100) NOT NULL,
    version VARCHAR(50) NOT NULL,
    parameters JSONB NOT NULL,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    active BOOLEAN DEFAULT TRUE,
    UNIQUE(strategy_id, parameter_group, version)
);

-- Create table for risk parameters
CREATE TABLE IF NOT EXISTS config.risk_parameters (
    id SERIAL PRIMARY KEY,
    exchange_id VARCHAR(50) NOT NULL,
    trading_pair VARCHAR(30) NOT NULL,
    strategy_id VARCHAR(100) NOT NULL,
    max_position_size NUMERIC NOT NULL,
    max_order_size NUMERIC NOT NULL,
    max_drawdown NUMERIC NOT NULL,
    max_exposure NUMERIC NOT NULL,
    additional_parameters JSONB,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    UNIQUE(exchange_id, trading_pair, strategy_id)
);

-- Create tables for position tracking
CREATE TABLE IF NOT EXISTS trading.positions (
    id SERIAL PRIMARY KEY,
    exchange_id VARCHAR(50) NOT NULL,
    trading_pair VARCHAR(30) NOT NULL,
    strategy_id VARCHAR(100) NOT NULL,
    asset VARCHAR(20) NOT NULL,
    quantity NUMERIC NOT NULL,
    average_entry_price NUMERIC NOT NULL,
    current_price NUMERIC,
    unrealized_pnl NUMERIC,
    realized_pnl NUMERIC DEFAULT 0,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    UNIQUE(exchange_id, trading_pair, strategy_id, asset)
);

-- Create tables for order tracking
CREATE TABLE IF NOT EXISTS trading.orders (
    id SERIAL PRIMARY KEY,
    client_order_id VARCHAR(100) NOT NULL UNIQUE,
    exchange_order_id VARCHAR(100),
    exchange_id VARCHAR(50) NOT NULL,
    trading_pair VARCHAR(30) NOT NULL,
    strategy_id VARCHAR(100) NOT NULL,
    signal_id VARCHAR(100),
    side VARCHAR(10) NOT NULL,
    order_type VARCHAR(20) NOT NULL,
    quantity NUMERIC NOT NULL,
    price NUMERIC,
    filled_quantity NUMERIC DEFAULT 0,
    average_fill_price NUMERIC,
    status VARCHAR(20) NOT NULL,
    time_in_force VARCHAR(10),
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
);

-- Create table for order fills
CREATE TABLE IF NOT EXISTS trading.fills (
    id SERIAL PRIMARY KEY,
    order_id INTEGER REFERENCES trading.orders(id),
    fill_id VARCHAR(100) NOT NULL,
    quantity NUMERIC NOT NULL,
    price NUMERIC NOT NULL,
    fee NUMERIC,
    fee_currency VARCHAR(20),
    timestamp TIMESTAMP WITH TIME ZONE NOT NULL,
    UNIQUE(order_id, fill_id)
);

-- Create index for common queries
CREATE INDEX idx_orders_strategy ON trading.orders(strategy_id, exchange_id, trading_pair);
CREATE INDEX idx_orders_status ON trading.orders(status);
CREATE INDEX idx_orders_timestamps ON trading.orders(created_at, updated_at);

-- Create roles and permissions
CREATE ROLE trading_read WITH LOGIN PASSWORD 'read_password';
CREATE ROLE trading_write WITH LOGIN PASSWORD 'write_password';

GRANT USAGE ON SCHEMA trading, config, metrics TO trading_read;
GRANT SELECT ON ALL TABLES IN SCHEMA trading, config, metrics TO trading_read;

GRANT USAGE ON SCHEMA trading, config, metrics TO trading_write;
GRANT ALL PRIVILEGES ON ALL TABLES IN SCHEMA trading, config, metrics TO trading_write;
GRANT USAGE ON ALL SEQUENCES IN SCHEMA trading, config, metrics TO trading_write;
