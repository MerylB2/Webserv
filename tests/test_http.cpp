#include "../includes/http/Request.hpp"
#include "../includes/http/Response.hpp"
#include "../includes/http/Router.hpp"
#include <iostream>
#include <cassert>

void test_request_parsing() {
    std::cout << "=== Test Request Parsing ===" << std::endl;

    Request req;

    // Simuler une requête GET
    std::string raw = "GET /index.html?name=test HTTP/1.1\r\n"
                      "Host: localhost\r\n"
                      "User-Agent: TestClient\r\n"
                      "\r\n";

    bool complete = RequestParser::parse(req, raw);

    std::cout << "Complete: " << (complete ? "yes" : "no") << std::endl;
    std::cout << "Method: " << req.method << std::endl;
    std::cout << "URI: " << req.uri << std::endl;
    std::cout << "Query: " << req.query_string << std::endl;
    std::cout << "Version: " << req.version << std::endl;
    std::cout << "Host header: " << req.headers["Host"] << std::endl;

    assert(complete == true);
    assert(req.method == "GET");
    assert(req.uri == "/index.html");
    assert(req.query_string == "name=test");
    assert(req.state == COMPLETE);

    std::cout << "[OK] Request parsing test passed!" << std::endl << std::endl;
}

void test_request_with_body() {
    std::cout << "=== Test Request with Body ===" << std::endl;

    Request req;

    std::string raw = "POST /upload HTTP/1.1\r\n"
                      "Host: localhost\r\n"
                      "Content-Length: 13\r\n"
                      "\r\n"
                      "Hello, World!";

    bool complete = RequestParser::parse(req, raw);

    std::cout << "Complete: " << (complete ? "yes" : "no") << std::endl;
    std::cout << "Method: " << req.method << std::endl;
    std::cout << "Body: " << req.body << std::endl;
    std::cout << "Content-Length: " << req.content_length << std::endl;

    assert(complete == true);
    assert(req.method == "POST");
    assert(req.body == "Hello, World!");
    assert(req.content_length == 13);

    std::cout << "[OK] Request with body test passed!" << std::endl << std::endl;
}

void test_request_chunked() {
    std::cout << "=== Test Chunked Request ===" << std::endl;

    Request req;

    std::string raw = "POST /data HTTP/1.1\r\n"
                      "Host: localhost\r\n"
                      "Transfer-Encoding: chunked\r\n"
                      "\r\n"
                      "5\r\n"
                      "Hello\r\n"
                      "6\r\n"
                      " World\r\n"
                      "0\r\n"
                      "\r\n";

    bool complete = RequestParser::parse(req, raw);

    std::cout << "Complete: " << (complete ? "yes" : "no") << std::endl;
    std::cout << "Body: " << req.body << std::endl;
    std::cout << "Is chunked: " << (req.is_chunked ? "yes" : "no") << std::endl;

    assert(complete == true);
    assert(req.body == "Hello World");
    assert(req.is_chunked == true);

    std::cout << "[OK] Chunked request test passed!" << std::endl << std::endl;
}

void test_response_build() {
    std::cout << "=== Test Response Build ===" << std::endl;

    Response res;
    ResponseBuilder::setStatus(res, 200);
    ResponseBuilder::setHeader(res, "Content-Type", "text/html");
    ResponseBuilder::setBody(res, "<h1>Hello</h1>");
    ResponseBuilder::build(res);

    std::cout << "Status: " << res.status_code << " " << res.status_message << std::endl;
    std::cout << "Is ready: " << (res.is_ready ? "yes" : "no") << std::endl;
    std::cout << "Send buffer size: " << res.send_buffer.size() << std::endl;
    std::cout << "\n--- Raw Response ---\n" << res.send_buffer << "-------------------" << std::endl;

    assert(res.status_code == 200);
    assert(res.is_ready == true);

    std::cout << "[OK] Response build test passed!" << std::endl << std::endl;
}

void test_response_error() {
    std::cout << "=== Test Response Error ===" << std::endl;

    Response res = ResponseBuilder::makeError(404);

    std::cout << "Status: " << res.status_code << " " << res.status_message << std::endl;
    std::cout << "\n--- Raw Response ---\n" << res.send_buffer << "-------------------" << std::endl;

    assert(res.status_code == 404);
    assert(res.status_message == "Not Found");

    std::cout << "[OK] Response error test passed!" << std::endl << std::endl;
}

void test_mime_types() {
    std::cout << "=== Test MIME Types ===" << std::endl;

    std::cout << ".html -> " << MimeTypes::getType(".html") << std::endl;
    std::cout << ".css  -> " << MimeTypes::getType(".css") << std::endl;
    std::cout << ".js   -> " << MimeTypes::getType(".js") << std::endl;
    std::cout << ".png  -> " << MimeTypes::getType(".png") << std::endl;
    std::cout << ".xyz  -> " << MimeTypes::getType(".xyz") << std::endl;

    assert(MimeTypes::getType(".html") == "text/html");
    assert(MimeTypes::getType(".css") == "text/css");
    assert(MimeTypes::getType(".png") == "image/png");

    std::cout << "\ngetExtension test:" << std::endl;
    std::cout << "/path/file.html -> " << MimeTypes::getExtension("/path/file.html") << std::endl;
    std::cout << "/path/file     -> " << MimeTypes::getExtension("/path/file") << std::endl;

    assert(MimeTypes::getExtension("/path/file.html") == ".html");

    std::cout << "[OK] MIME types test passed!" << std::endl << std::endl;
}

void test_http_status() {
    std::cout << "=== Test HTTP Status ===" << std::endl;

    std::cout << "200 -> " << HttpStatus::getMessage(200) << std::endl;
    std::cout << "404 -> " << HttpStatus::getMessage(404) << std::endl;
    std::cout << "500 -> " << HttpStatus::getMessage(500) << std::endl;

    assert(HttpStatus::getMessage(200) == "OK");
    assert(HttpStatus::getMessage(404) == "Not Found");
    assert(HttpStatus::getMessage(500) == "Internal Server Error");

    std::cout << "[OK] HTTP status test passed!" << std::endl << std::endl;
}

int main() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "       WEBSERV HTTP TESTS" << std::endl;
    std::cout << "========================================\n" << std::endl;

    test_request_parsing();
    test_request_with_body();
    test_request_chunked();
    test_response_build();
    test_response_error();
    test_mime_types();
    test_http_status();

    std::cout << "========================================" << std::endl;
    std::cout << "       ALL TESTS PASSED!" << std::endl;
    std::cout << "========================================\n" << std::endl;

    return 0;
}
