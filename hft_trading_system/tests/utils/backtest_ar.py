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
    
    # Check if the profit array is too short or has no variation
    if len(profit) < 5 or np.std(profit) == 0:
        return 0
    
    returns = safe_returns(profit)  
    
    # Adjust returns for risk-free rate if needed (Sharpe ratio typically assumes zero risk-free)
    excess_returns = returns - risk_free_rate
    
    # Calculate the average excess daily return and standard deviation of returns
    avg_return = np.mean(excess_returns)
    std_return = np.std(excess_returns)
    
    # Calculate and return the Sharpe ratio
    sharpe_ratio = avg_return / std_return if std_return != 0 else 0
    return sharpe_ratio

#test plot 
def plot_positions(prices, long_position, short_position):
    import matplotlib.pyplot as plt
    plt.figure(figsize=(14, 7))
    plt.plot(prices, label='Price')
    plt.plot(np.ma.masked_where(long_position == 0, long_position) * prices, 'g^', markersize=8, label='Long')
    plt.plot(np.ma.masked_where(short_position == 0, short_position) * prices, 'rv', markersize=8, label='Short')
    plt.legend()
    plt.show()
#plot the profit
def plot_profit(profit):
    import matplotlib.pyplot as plt
    plt.figure(figsize=(14, 7))
    plt.plot(profit, label='Profit')
    plt.legend()
    plt.show()
def backtest_strategy(prices, threshold1, threshold2, initial_capital=10000, position_size=100, transaction_cost=0.001):
   
    # Initialize variables for tracking positions and profits
    position = 0  # -1 for short, 1 for long, 0 for no position
    capital = initial_capital
    position_price = 0
    n = len(prices)
    #an array to store the profit
    profit = np.zeros(n) + initial_capital
    accumulated_profit = np.zeros(n)
    transition = 0 # 0 for no transition, 1 for transitioning
    #variables to store position to plot later
    long_position = np.zeros(n)
    short_position = np.zeros(n)

    for i in range(10, n, 1):
        # Skip the first 200 prices, as we need 200 prices to calculate log returns
        if i < 200:
            profit[i] = capital
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
        #calculate %atr
        atr = std_price / mean_price

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
            #store the profit
            profit[i] = capital
        # Determine strategy based on phi
        price = prices[i]
        if phi < threshold1:
            #just bypass
            # continue
            if price <= mean_price - 2*std_price and price < prices[i - 1] and atr > 0.002:
                # and price < prices[i - 1]:
                print(f"Mean reversion: Long position (price below mean) at price {price}")
                position = 1 
                position_price = price 
            elif price >= mean_price + 2*std_price and price > prices[i - 1] and atr > 0.002:
                #and price > prices[i - 1]:
                print(f"Mean reversion: Short position (price above mean) at price {price}")
                position = -1 
                position_price = price
            #condition to close the position
            else:
                position = 0

        # Momentum strategy
        elif phi > threshold2:
            #just bypass
            continue
            if price > mean_price and price > prices[i - 1]:
                print(f"Momentum: Long position (price rising) at price {price}")
                position = 1 
                position_price = price
            elif price < mean_price and price < prices[i - 1]:
                print(f"Momentum: Short position (price falling) at price {price}")
                position = -1  
                position_price = price
            #condition to close the position
            else:
                position = 0
        else:
            # print(f"No strong signal at price {price}. Maintaining current position.")
            position = 0  # No position or maintain current one
        #store the profit
        profit[i] = capital
        #calculate the accumulated profit
        accumulated_profit[i] += accumulated_profit[i-1] + profit[i] - profit[i-1] 

        
        if position == 1:
            long_position[i] = 1
        elif position == -1:
            short_position[i] = 1
    #end of loop


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

    #plot the profit
    plot_profit(profit)
    plot_profit(accumulated_profit)
    #plot the position
    plot_positions(prices, long_position, short_position)

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
    thresholds2_l = np.linspace(0.001, 0.2, steps)
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
    #run backtest_strategy SOLUSDT_1m
    data = pd.read_csv('AAVEUSDT_1m.csv')
    prices = data['Close'].values

    # Randomly select a portion of the data for optimization
    datalen= 10000
    index = np.random.randint(0, len(prices) - datalen)
    selected_prices = prices[index:index + datalen]
   # optimize parasm
    # profit_percentages_optimal_threshold1, profit_percentages_optimal_threshold2 , sharpe_optimal_threshold1, sharpe_optimal_threshold2 = manual_optimizer(selected_prices)

    # profit_percentage, sharpe_ratio = backtest_strategy(selected_prices, profit_percentages_optimal_threshold1, profit_percentages_optimal_threshold2)
    # print(f"Profit percentage: {profit_percentage:.2f}%")
    # print(f"Sharpe ratio: {sharpe_ratio:.4f}")

    # profit_percentage, sharpe_ratio = backtest_strategy(selected_prices, sharpe_optimal_threshold1, sharpe_optimal_threshold2)
    # print(f"Profit percentage: {profit_percentage:.2f}%")
    # print(f"Sharpe ratio: {sharpe_ratio:.4f}")

    #run test on pre value
    profit_percentage, sharpe_ratio = backtest_strategy(selected_prices, -0.07, 0.2)
    print(f"Profit percentage: {profit_percentage:.2f}%")
    print(f"Sharpe ratio: {sharpe_ratio:.4f}")
    '''
    Optimal threshold1 for sharpe ratio: -0.4, Optimal threshold2 for sharpe ratio: 0.18
Optimal threshold1 for profit percentage: -0.07, Optimal threshold2 for profit percentage: 0.2
    Optimal threshold1 for sharpe ratio: -0.1, Optimal threshold2 for sharpe ratio: 0.08944444444444445
    Profit percentage: 1.69%
    Sharpe ratio: 6.9576
    Profit percentage: -4.51%
    Sharpe ratio: 6.9937

    '''