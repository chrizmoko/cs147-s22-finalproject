#ifndef HELLOWORLD_NETWORK_HPP
#define HELLOWORLD_NETWORK_HPP


#include <Wifi.h>
#include <HttpClient.h>
#include <vector>
#include <ArduinoJson.h>


class ApplicationNetworkException
{
public:
    ApplicationNetworkException(const char* reason);
    ApplicationNetworkException(const String& reason);

    /**
     * Returns a string describing why the exception occurred.
     */
    const char* reason() const noexcept;

private:
    const String reason_;
};


class ApplicationNetworkClient
{
public:
    /**
     * This struct holds the retrieved message attributes
     */
    struct message_map {
        String macAddress;
        String content;
        String time;
    };
    /**
     * Creates a network client that will connect to a server. Any http requests
     * made by this client will be directed to that server and port.
     */
    ApplicationNetworkClient(const char* address, short port);

    /**
     * Destructor for the ApplicationNetworkClient instance.
     */
    ~ApplicationNetworkClient();

    /**
     * Returns the address that this client is connecting to.
     */
    const char* getAddress() noexcept;

    /**
     * Returns the port that this client is connecting to.
     */
    short getPort() noexcept;

    /**
     * Returns the Wifi client.
     */
    WiFiClient& getWifiClient() noexcept;

    /**
     * Notifies the server that the device exists. The device will be able to
     * send and receive messages in the application network. If the device is
     * already registered, then no action is taken.
     */
    void makeVisible();

    /**
     * Notifies the server that the device no longer wishes to remain visible to
     * the server. The device will no longer be able to send or receive messages
     * in the application network. Moreover, any pending messages of the device
     * will be cleared.
     */
    void makeInvisible();

    /**
     * Sends a text message to the server in a HTTP POST request. Headers of the
     * request include "userId" (the user identification) and "message" (the
     * message itself).
     * 
     * NOTICE: Sending a message will be broadcasted to all other devices
     * visible to the server.
     */
    void sendMessage(const char* message);

    /**
     * Returns the number of messages the server has stored but not sent to the
     * client. This sends an HTTP request to the server.
     */
    int countPendingMessages();

    /**
     * If there are messages that have not yet been sent to the client, then
     * those messages will be sent. The number of pending messages to be sent to
     * the client are limited by the amount requested. There may be pending
     * messages afterwards. This sends an HTTP request to the server.
     */
    void fetchPendingMessages(int amount);

    /**
     * Returns a vector of message_maps representing messages that the client has
     * fetched from the server. All the messages fetched will be returned and no
     * longer managed by the ApplicationNetworkClient.
     */
    std::vector<message_map> getFetchedMessages();

private:
    const char* endpoint_address_;
    const short endpoint_port_;
    WiFiClient wifi_client_;
    std::vector<message_map> pending_messages_;
};


/**
 * Used since the server only requires http POST requests. This requires the
 * content body of the request to be in form data.
 */
class FormDataFormatter
{
public:
    /**
     * Creates a formatter to format key-value pairs in the proper format.
     */
    FormDataFormatter();
    
    /**
     * Adds a key-value pair to the formatter.
     */
    void addPair(const char* key, const char* value);

    /**
     * Converts the pairs into a string.
     */
    String toString();

private:
    int key_value_pairs_;
    String output_;
};


#endif