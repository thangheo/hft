import pandas as pd
import numpy as np
from statsmodels.tsa.ar_model import AutoReg
from scipy.optimize import minimize
 
import numpy as np

def safe_returns(profit):
    # Replace zero or near-zero values in profit[:-1] to avoid divide-by-zero errors
    adjusted_profit = np.where(np.abs(profit[:-1]) < 1e-10, 1e-10, profit[:-1])
    
    # Calculate returns with safe division
    returns = np.diff(profit) / adjusted_profit
    
    # Remove any NaN or infinite values in the returns
    returns = np.nan_to_num(returns, nan=0.0, posinf=0.0, neginf=0.0)
    
    return returns

def calculate_sharpe_ratio(profit, risk_free_rate=0):
    # Remove NaN or infinite values from the profit array
    profit = np.nan_to_num(profit, nan=0.0, posinf=0.0, neginf=0.0)
    
    # Check if the profit array is too short or has no variation
    if len(profit) < 5 or np.std(profit) == 0:
        return 0
    
    returns = np.where(np.abs(profit[:-1]) < 1e-10, 1e-10, profit[:-1])

    
    # Handle edge case where all returns are zero or close to zero
    if np.std(returns) == 0:
        return 0
    
    # Adjust returns for risk-free rate if needed (Sharpe ratio typically assumes zero risk-free)
    excess_returns = returns - risk_free_rate
    
    # Calculate the average excess daily return and standard deviation of returns
    avg_return = np.mean(excess_returns)
    std_return = np.std(excess_returns)
    
    # Calculate and return the Sharpe ratio
    sharpe_ratio = avg_return / std_return if std_return != 0 else 0
    return sharpe_ratio


def backtest_strategy(prices, threshold1, threshold2, initial_capital=10000, position_size=100, transaction_cost=0.001):
   
    # Initialize variables for tracking positions and profits
    position = 0  # -1 for short, 1 for long, 0 for no position
    capital = initial_capital
    position_price = 0
    n = len(prices)
    #an array to store the profit
    profit = np.zeros(n)

    for i in range(10, n, 10):
        # Skip the first 200 prices, as we need 200 prices to calculate log returns
        if i < 200:
            continue

        # Calculate log returns for the last 200 prices
        past_prices = prices[i - 200:i]
        returns = np.log(past_prices[1:] / past_prices[:-1])

        # Fit AR(1) model on the calculated returns
        model = AutoReg(returns, lags=1, trend='c').fit()
        phi = model.params[1]  # Extract the phi (lag-1 coefficient)
        #calculate rolling mean
        mean_price = np.mean(past_prices)
        #calculate rolling std
        std_price = np.std(past_prices)

        # Close any existing position if transitioning between strategies
        if position != 0:
            if position == 1:  # Long position
                trade_value = (prices[i] - position_price) * position_size
                capital += trade_value
                capital -= abs(trade_value) * transaction_cost  # Apply transaction cost
            elif position == -1:  # Short position
                trade_value = (position_price - prices[i]) * position_size
                capital += trade_value
                capital -= abs(trade_value) * transaction_cost  # Apply transaction cost
            position = 0  # Reset position after closing

        # Determine strategy based on phi
        price = prices[i]
        if phi < threshold1:
            if price < mean_price - 2*std_price and price < prices[i - 1]:
                # print(f"Mean reversion: Long position (price below mean) at price {price}")
                position = 1  # Go long (buy), expecting the price to revert upward toward the mean
            elif price > mean_price + 2*std_price and price > prices[i - 1]:
                # print(f"Mean reversion: Short position (price above mean) at price {price}")
                position = -1  # Go short (sell), expecting the price to revert downward toward the mean
            #condition to close the position
            else:
                position = 0
                # if position != 0:
                #     if position == 1:

        # Momentum strategy
        elif phi > threshold2:
            if price > mean_price and price > prices[i - 1]:
                # print(f"Momentum: Long position (price rising) at price {price}")
                position = 1  # Go long (buy), expecting the price to continue rising
            elif price < mean_price and price < prices[i - 1]:
                # print(f"Momentum: Short position (price falling) at price {price}")
                position = -1  # Go short (sell), expecting the price to continue falling
            #condition to close the position
            else:
                position = 0
        else:
            # print(f"No strong signal at price {price}. Maintaining current position.")
            position = 0  # No position or maintain current one
        #store the profit
        profit[i] = capital
    # Close the final position at the last price point if needed
    if position != 0:
        if position == 1:  # Close long position
            trade_value = (prices[-1] - position_price) * position_size
            capital += trade_value
            capital -= abs(trade_value) * transaction_cost  # Apply transaction cost
        elif position == -1:  # Close short position
            trade_value = (position_price - prices[-1]) * position_size
            capital += trade_value
            capital -= abs(trade_value) * transaction_cost  # Apply transaction cost

    # Calculate the final profit
    final_profit = capital - initial_capital

    # Calculate profit percentage based on the initial capital
    profit_percentage = (final_profit / initial_capital) * 100
    #calculate sharpe ratio
    sharpes_ratio = calculate_sharpe_ratio(profit)

    return profit_percentage , sharpes_ratio

# Objective function for optimization
def objective_function(params, prices):
    threshold1, threshold2 = params
    profit_percentage , sharpes_ratio = backtest_strategy(prices, threshold1, threshold2)
    # Since minimize() minimizes the objective, return the negative profit to maximize it
    return -sharpes_ratio

# Load the historical data and run the optimization
def optimize_strategy(csv_file):
    # Load historical price data
    data = pd.read_csv(csv_file)
    prices = data['Close'].values
    
    # Randomly select a portion of the data for optimization
    index = np.random.randint(0, len(prices) - 10000)
    selected_prices = prices[index:index + 10000]

    # Set initial guesses for thresholds
    initial_guess = np.random.uniform(-1, 1, size=2)

    # Use scipy's minimize function to optimize threshold1 and threshold2
    result = minimize(
        objective_function, 
        initial_guess, 
        args=(selected_prices,), 
        bounds=[(-1, 1), (0, 2)],  # Widen the bounds
        method='L-BFGS-B', 
        tol=1e-8,           # Set tolerance for convergence
        options={'disp': True, 'maxiter': 500}  # Increase max iterations
    )

    optimal_threshold1, optimal_threshold2 = result.x
    print(f"Optimal threshold1: {optimal_threshold1}, Optimal threshold2: {optimal_threshold2}")
    #print final profit
    final_profit = -result.fun
    print(f"Final profit: {final_profit:.2f}")
    # Calculate profit percentage based on the initial capital
    profit_percentage = (final_profit / 10000) * 100
    print(f"Profit percentage: {profit_percentage:.2f}%")

#manual optimize with loop
def manual_optimizer(prices):

    sharpe_ratios = []
    profit_percentages = []
    steps = 10
    thresholds1_l = np.linspace(-0.1, -0.01, steps)
    thresholds2_l = np.linspace(0.01, 0.2, steps)
    for threshold1 in thresholds1_l:
        for threshold2 in thresholds2_l:
            profit_percentage, sharpe_ratio = backtest_strategy(prices, threshold1, threshold2)
            profit_percentages.append(profit_percentage)
            sharpe_ratios.append(sharpe_ratio)
            print(f"Threshold1: {threshold1}, Threshold2: {threshold2}, Profit percentage: {profit_percentage:.2f}%, Sharpe ratio: {sharpe_ratio:.4f}")

    max_profit_percentage = max(profit_percentages)
    max_index = profit_percentages.index(max_profit_percentage)
    profit_percentages_optimal_threshold1 = thresholds1_l[max_index // steps]
    profit_percentages_optimal_threshold2 = thresholds2_l[max_index % steps]

    max_sharpe_ratio = max(sharpe_ratios)
    max_index = sharpe_ratios.index(max_sharpe_ratio)
    sharpe_optimal_threshold1 = thresholds1_l[max_index // steps]
    sharpe_optimal_threshold2 = thresholds2_l[max_index % steps]


    print(f"Optimal threshold1 for profit percentage: {profit_percentages_optimal_threshold1}, Optimal threshold2 for profit percentage: {profit_percentages_optimal_threshold2}")
    print(f"Optimal threshold1 for sharpe ratio: {sharpe_optimal_threshold1}, Optimal threshold2 for sharpe ratio: {sharpe_optimal_threshold2}")
    return profit_percentages_optimal_threshold1, profit_percentages_optimal_threshold2 , sharpe_optimal_threshold1, sharpe_optimal_threshold2


if __name__ == '__main__':
    # optimize_strategy('AAVEUSDT_1m.csv')
    # exit()
    #run backtest_strategy
    data = pd.read_csv('AAVEUSDT_1m.csv')
    prices = data['Close'].values

    # Randomly select a portion of the data for optimization
    index = np.random.randint(0, len(prices) - 10000)
    selected_prices = prices[index:index + 10000]
   # optimize parasm
    profit_percentages_optimal_threshold1, profit_percentages_optimal_threshold2 , sharpe_optimal_threshold1, sharpe_optimal_threshold2 = manual_optimizer(selected_prices)

    profit_percentage, sharpe_ratio = backtest_strategy(selected_prices, profit_percentages_optimal_threshold1, profit_percentages_optimal_threshold2)
    print(f"Profit percentage: {profit_percentage:.2f}%")
    print(f"Sharpe ratio: {sharpe_ratio:.4f}")

    profit_percentage, sharpe_ratio = backtest_strategy(selected_prices, sharpe_optimal_threshold1, sharpe_optimal_threshold2)
    print(f"Profit percentage: {profit_percentage:.2f}%")
    print(f"Sharpe ratio: {sharpe_ratio:.4f}")

    #run test on pre value
    # profit_percentage, sharpe_ratio = backtest_strategy(selected_prices, -0.18, 0.18)
    # print(f"Profit percentage: {profit_percentage:.2f}%")
    # print(f"Sharpe ratio: {sharpe_ratio:.4f}")
    '''
    Optimal threshold1 for sharpe ratio: -0.4, Optimal threshold2 for sharpe ratio: 0.18

    '''