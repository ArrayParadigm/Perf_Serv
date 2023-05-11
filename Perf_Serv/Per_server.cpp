// Server.cpp
#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <pqxx/pqxx>

using boost::asio::ip::tcp;

int main(int argc, char* argv[])
{
    try
    {
        // Initialize the networking library
        boost::asio::io_service io_service;

        // Create a TCP acceptor to listen for incoming connections
        tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), 1234));

        // Initialize the PostgreSQL connection
        pqxx::connection conn("dbname=mydatabase user=myuser password=mypassword");

        // Accept incoming connections and handle them one by one
        while (true)
        {
            // Wait for a new client to connect
            tcp::socket socket(io_service);
            acceptor.accept(socket);

            // Read data from the client
            boost::asio::streambuf buffer;
            boost::asio::read_until(socket, buffer, '\n');
            std::string message = boost::asio::buffer_cast<const char*>(buffer.data());
            message.erase(message.size() - 1);

            // Print the received message to the console
            std::cout << "Received message: " << message << std::endl;

            // Parse the message to extract the username and password
            std::string username = message.substr(0, message.find(':'));
            std::string password = message.substr(message.find(':') + 1);

            // Authenticate the user
            bool authenticated = false;
            try
            {
                pqxx::work txn(conn);
                pqxx::result res = txn.exec(
                    "SELECT COUNT(*) FROM users WHERE username = " +
                    txn.quote(username) +
                    " AND password = crypt(" +
                    txn.quote(password) +
                    ", password)"
                );
                authenticated = res[0][0].as<int>() == 1;
                txn.commit();
            }
            catch (const pqxx::pqxx_exception& e)
            {
                std::cerr << "Authentication error: " << e.what() << std::endl;
            }

            // Send the authentication result to the client
            boost::asio::write(socket, boost::asio::buffer(authenticated ? "OK\n" : "ERROR\n"));
        }
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
