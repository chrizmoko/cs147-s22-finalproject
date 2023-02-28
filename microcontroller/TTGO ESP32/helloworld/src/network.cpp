#include "network.hpp"

#include <WiFi.h>
#include <HttpClient.h>
#include <vector>


static void DEBUG(String message)
{
    Serial.print("[NETWORK_DEBUG] ");
    Serial.println(message);
}


/**
 * Writes to the server and returns the response received from the server.
 */
static String writeToServer(ApplicationNetworkClient& client,
                            FormDataFormatter& form,
                            const char* url_path);

/**
 * Outputs an http response into a string.
 */
static String getResponseFromServer(HttpClient& http_client);


/******************************************************************************/
/* ApplicationNetworkException                                                */
/******************************************************************************/


ApplicationNetworkException::ApplicationNetworkException(const char* reason)
    : reason_(reason)
{
}


ApplicationNetworkException::ApplicationNetworkException(const String& reason)
    : reason_(reason)
{
}


const char* ApplicationNetworkException::reason() const noexcept
{
    return reason_.c_str();
}


/******************************************************************************/
/* ApplicationNetworkClient                                                   */
/******************************************************************************/


ApplicationNetworkClient::ApplicationNetworkClient(
    const char* address,
    short port
)
    : endpoint_address_(address), endpoint_port_(port), wifi_client_()
{
    wifi_client_.setTimeout(10);
}


ApplicationNetworkClient::~ApplicationNetworkClient()
{
}


const char* ApplicationNetworkClient::getAddress() noexcept
{
    return endpoint_address_;
}


short ApplicationNetworkClient::getPort() noexcept
{
    return endpoint_port_;
}


WiFiClient& ApplicationNetworkClient::getWifiClient() noexcept
{
    return wifi_client_;
}


void ApplicationNetworkClient::makeVisible()
{
    const char* path = "/api/device/register";

    FormDataFormatter form_data;
    form_data.addPair("macAddress", WiFi.macAddress().c_str());
    // form_data.addPair("username", "my_username");

    DEBUG("makeVisible() -> /api/device/register");
    DEBUG(form_data.toString());
    writeToServer(*this, form_data, path);
}


void ApplicationNetworkClient::makeInvisible()
{
    const char* path = "/api/device/unregister";
    
    FormDataFormatter form_data;
    form_data.addPair("macAddress", WiFi.macAddress().c_str());
    
    DEBUG("makeInvisible() -> /api/device/unregister");
    DEBUG(form_data.toString());
    writeToServer(*this, form_data, path);
}


void ApplicationNetworkClient::sendMessage(const char* message)
{
    const char* path = "/api/device/message/receive";
    HttpClient http_client(wifi_client_);

    FormDataFormatter form_data;
    form_data.addPair("macAddress", WiFi.macAddress().c_str());
    form_data.addPair("message", message);

    DEBUG("sendMessage() -> /api/device/message/receive");
    DEBUG(form_data.toString());
    writeToServer(*this, form_data, path);
}


int ApplicationNetworkClient::countPendingMessages()
{
    const char* path = "/api/device/message/pending/count";
    HttpClient http_client(wifi_client_);

    FormDataFormatter form_data;
    form_data.addPair("macAddress", WiFi.macAddress().c_str());

    // DEBUG("countPendingMessages() -> /api/device/message/pending/count");
    // DEBUG(form_data.toString());
    String response = writeToServer(*this, form_data, path);
    DynamicJsonDocument doc(256);
    deserializeJson(doc, response);
    int count = doc["count"];

    DEBUG("\treceived: " + String(count));
    return count;
}


void ApplicationNetworkClient::fetchPendingMessages(int amount = 1)
{
    const char* path = "/api/device/message/pending/get";
    HttpClient http_client(wifi_client_);

    String amount_str = String(amount);

    FormDataFormatter form_data;
    form_data.addPair("macAddress", WiFi.macAddress().c_str());
    form_data.addPair("limit", amount_str.c_str());

    DEBUG("fetchPendingMessages() -> /api/device/message/pending/get");
    DEBUG(form_data.toString());
    String response = writeToServer(*this, form_data, path);

    DEBUG("\tcalled response" + String(response));


    DynamicJsonDocument doc(256);
    deserializeJson(doc, response);
    int count = doc["count"];

    DEBUG("\tmessages received: " + String(count));

    // DynamicJsonDocument msg_doc(512);
    // deserializeJson(msg_doc, ;
    JsonArray messages = doc["messages"];

    DEBUG("\tsuccessfully initializzed message JsonArray");

    for (int i = 0; i < count; i++){
        struct ApplicationNetworkClient::message_map mm;
        JsonObject message = messages[i];
        const char* macAddress = message["macAddress"];
        const char* content = message["content"];
        const char* time = message["time"];
        mm.macAddress = String(macAddress);
        mm.content = String(content);
        mm.time = String(time);

        pending_messages_.push_back(mm);
    }
}

std::vector<ApplicationNetworkClient::message_map> ApplicationNetworkClient::getFetchedMessages()
{
    std::vector<ApplicationNetworkClient::message_map> output = std::vector<ApplicationNetworkClient::message_map>(pending_messages_);
    pending_messages_.clear();
    return output;
}


/******************************************************************************/
/* FormDataFormatter                                                          */
/******************************************************************************/


FormDataFormatter::FormDataFormatter()
    : key_value_pairs_(0), output_()
{
}


void FormDataFormatter::addPair(const char* key, const char* value)
{
    if (key_value_pairs_ > 0)
    {
        output_ += "&";
    }

    output_ += key;
    output_ += "=";
    output_ += value;

    key_value_pairs_++;
}


String FormDataFormatter::toString()
{
    return String(output_);
}


/******************************************************************************/
/* static functions                                                           */
/******************************************************************************/


String writeToServer(ApplicationNetworkClient& client,
                   FormDataFormatter& form,
                   const char* url_path)
{
    HttpClient http_client(client.getWifiClient());
    String content = form.toString();

    http_client.beginRequest();
    http_client.startRequest(client.getAddress(), client.getPort(), url_path,
                             "POST", NULL);
    http_client.sendHeader("Content-Type", "application/x-www-form-urlencoded");
    http_client.sendHeader("Content-Length", content.length());
    for (int i = 0; i < content.length(); i++)
    {
        http_client.write(content.charAt(i));
    }
    http_client.endRequest();

    // Check for OK response from server
    String server_response = getResponseFromServer(http_client);
    if (server_response.indexOf("OK\n") != 0)
    {
        Serial.println("Bad request");
    }

    http_client.stop();
    return server_response;
}


String getResponseFromServer(HttpClient& http_client)
{
    int error = http_client.responseStatusCode();
    if (error != 200)
    {
        // throw ApplicationNetworkException(
        //     String("Getting response failed with HTTP code") + String(error)
        // );
        DEBUG("[ERROR] Getting response failed with HTTP code " + String(error));
    }

    error = http_client.skipResponseHeaders();
    if (error < 0)
    {
        // throw ApplicationNetworkException("Failed to skip response headers");
        DEBUG("[ERROR] Failed to skip response errors; code " + String(error));
    }

    // Read response body
    String response_message = String();
    while (http_client.available())
    {
        response_message += (char)http_client.read();
    }
    return response_message;
}