# Deribit - Order Execution and Management System
This is an implementation of an Order Execution and Management System which functions on WebSocket and is built on C++.

## Steps (Linux)
1. Generate Credentials: Create an account on https://test.deribit.com/ and generate your API credentials - `client_id` and `client_secret`.
2. Clone this repository
    ```bash
    git clone https://github.com/ppl-call-me-tima/deribit.git
    cd deribit
    ```
3. Install the websocketpp library and its dependencies
    ```bash
    sudo apt update
    sudo apt install libwebsocketpp-dev
    sudo apt install libssl-dev
    sudo apt install libboost-all-dev
    ```
4. Install the nlohmann/json library.
    ```bash
    sudo apt install nlohmann-json3-dev
    ```
5. Run the Makefile and upon completion, run the executable.
    ```bash
    make
    ./main
    ```
## List of Commands
1. **show**: Shows the metdata for the WebSocket connection along with incoming/outgoing messages along with the error logs if any.
2. **auth**: Authenticates user which is required for private methods of the API.<br>
Parameters: `client_id`, `client_secret`
3. **buy**: Executes a buy request of a particular instrument, amount, type along with a label.<br>
Parameters: `instrument_name`, `amount`, `type`, `label`
4. **sell**: Executes a sell request of a particular instrument, amount, type along with a label.<br>
Parameters: `instrument_name`, `amount`, `type`, `label`
5. **cancel**: Cancels an order with a particular order ID.<br>
Parameters: `order_id`
6. **cancel all**: Cancels all the currnet open orders.
7. **edit**: Edits the amount or price for a current opened order with a particular order ID.<br>
Placeholders: `order_id`, `amount`, `price`
8. **orders**: Shows the information of all the current opened orders.
9. **orderbook**: Shows the orderbook for a particular instrument upto a specified depth.<br>
Parameters: `instrument_name`, `depth`
10. **positions**: Shows the current position of the user for a particular currency.<br>
Parameters: `currency`
11. **subscribe**: Subscribes to a particular instrument channel.<br>
Parameters: `channel`, `interval`
12. **unsubscribe**: Unsubsribe from a particular instrument channel.<br>
Paramters: `channel`, `interval`
13. **tgl**: Toggles the continuous incoming of responses of the subscribed channel.<br> 
14. **quit**: Close the WebSocket connection.