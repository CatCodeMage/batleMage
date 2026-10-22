#include "webPage.h"
#include <string>
#include <iostream>
#include <nlohmann/json.hpp>
#include "jsonManager.h"

using json = nlohmann::json;

const int MAX_CONNECTIONS = 4;

std::string urlDecode(const std::string& s) {
    std::string out;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '+') out += ' ';
        else if (s[i] == '%' && i + 2 < s.size()) {
            int v = std::stoi(s.substr(i + 1, 2), nullptr, 16);
            out += static_cast<char>(v);
            i += 2;
        }
        else out += s[i];
    }
    return out;
}

std::map<std::string, std::string> parseForm(const std::string& body) {
    std::map<std::string, std::string> out;
    std::stringstream ss(body);
    std::string pair;
    while (std::getline(ss, pair, '&')) {
        auto eq = pair.find('=');
        if (eq == std::string::npos) continue;
        out[urlDecode(pair.substr(0, eq))] = urlDecode(pair.substr(eq + 1));
    }
    return out;
}

std::string builPage() {
    std::vector<std::string> players = getPlayers();

    std::string options = "";

    for (auto& player : players) {
        options += "<option value=\"" + player + "\">" + player + "</option>";
    }

    std::string html = R"(<!DOCTYPE html>
    <html>
        <head>
            <meta charset="utf-8"><title>Hechizos</title>
            <style>
              body { font-family: sans-serif; margin: 2em; }
              table { border-collapse: collapse; margin-bottom: 2em; }
              th, td { border: 1px solid #ccc; padding: 4px 8px; text-align: left; }
              th { background: #eee; }
            </style>
        </head>
        <body>
            <h2>Opciones de juego</h2>
            <form action="/submit" method="POST">
                <label for="combatType">Selecciona el tipo de simulacion</label><br>
                <select id="combatType" name="combatType">
                    <option value="1v1">1v1</option>
                    <option value="horde">Horda</option>
                </select><br>
                <label for="player_1">Selecciona al jugador 1</label><br>
                <select id="player_1" name="player_1">)";

    html += options;
    
    html += R"(
        </select><br>
        <label for="player_2">Selecciona al jugador 2</label><br>
        <select id="player_2" name="player_2" >
        )";
    
    html += options;

    html += R"(
                </select><br>
                <button type="submit">Iniciar simulacion</button>
            </form>
        </body>
    </html>
    )";

    return html;
}

void openBrowser(const std::string& url) {
    std::system(("start \"\" \"" + url + "\"").c_str());
}

bool saveCombatMetadata(const std::map<std::string, std::string>& form) {
    try {

        json combatMetadata = {
            {"metadata", {
                {"combatType", ""}
            }},
            {"map", ""},
            {"players", {
                {"player_1", {
                    {"type", ""},
                    {"position", {0, 0, 0}}
                }},
                {"player_2", {
                    {"type", ""},
                    {"position", {0, 0, 0}}
                }}
            }},
            {"spells", ""},
            {"frames", ""}
                };

        combatMetadata["metadata"]["combatType"] = form.at("combatType");
        combatMetadata["map"] = "../db/map.json";
        combatMetadata["players"]["player_1"]["type"] = form.at("player_1");
        combatMetadata["players"]["player_1"]["position"] = { -20, 0, -20 };
        combatMetadata["players"]["player_2"]["type"] = form.at("player_2");
        combatMetadata["players"]["player_2"]["position"] = { 20, 0, 20 };
        combatMetadata["spells"] = "../db/spells.json";

        std::ofstream combat(DESTINATION_FILE);

        if (!combat.is_open())
        {
            return false;
        }

        combat << combatMetadata.dump(4);


        return true;
    }
    catch (const std::exception& e) {
        std::cout << e.what() << std::endl;

        return false;
    }

}

bool sendForm() {

    // Obtener hrm que sera la pagina
    std::string html = builPage();

    // Preparar pàgina, hacer que escuche el servidor connexiones tcp por el puerto 8080
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return false;

    socket_t server = socket(AF_INET, SOCK_STREAM, 0);

    if (server == INVALID_SOCKET) {
        WSACleanup();

        return false;
    }

    int opt = 1;
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons(PORT);

    if (bind(server, (sockaddr*)&addr, sizeof(addr)) < 0) {
        WSACleanup();

        return false;
    }

    listen(server, 4);

    // Abrir navegador con la pagina
    openBrowser("http://localhost:" + std::to_string(PORT));

    bool success = false;

    for (int i = 0; i < MAX_CONNECTIONS && !success; i++)
    {
        socket_t client = accept(server, nullptr, nullptr);

        if (client == INVALID_SOCKET)
        {
            continue;
        }

        std::string request;
        char buff[4096];
        int recieved;
        size_t headerEnd = std::string::npos;

        while ((recieved = recv(client, buff, sizeof(buff), 0)) > 0)
        {
            request.append(buff, recieved);

            if (headerEnd == std::string::npos)
            {
                headerEnd = request.find("\r\n\r\n");
            }

            if (headerEnd != std::string::npos)
            {
                size_t cl = 0;
                auto p = request.find("Content-Length:");
                if (p != std::string::npos)
                    cl = std::stoul(request.substr(
                        p + 15, request.find("\r\n", p) - p - 15));
                if (request.size() - (headerEnd + 4) >= cl) break;
            }
        }

        std::string method, path;

        std::istringstream(request.substr(0, request.find("\r\n")))
            >> method >> path;

        std::string body = (headerEnd != std::string::npos)
            ? request.substr(headerEnd + 4) : "";

        std::string response;

        if (method == "GET" && path == "/") 
        {
            response = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\n"
                "Content-Length: " + std::to_string(html.size()) + "\r\n"
                "Connection: close\r\n\r\n" + html;
        }
        else if (method == "POST" && path == "/submit") 
        {
            auto form = parseForm(body);
            if (saveCombatMetadata(form)) 
            {
                success = true;
                std::string ok = "<h1>Guardado</h1>";
                response = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\n"
                    "Content-Length: " + std::to_string(ok.size()) + "\r\n"
                    "Connection: close\r\n\r\n" + ok;
            }
            else 
            {
                std::string err = "<h1>Error guardando</h1>";
                response = "HTTP/1.1 500 Internal Server Error\r\n"
                    "Content-Type: text/html; charset=utf-8\r\n"
                    "Content-Length: " + std::to_string(err.size()) + "\r\n"
                    "Connection: close\r\n\r\n" + err;
            }
        }
        else 
        {
            response = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n"
                "Connection: close\r\n\r\n";
        }

        send(client, response.c_str(), (int)response.size(), 0);
        CLOSE_SOCKET(client);
    }

    CLOSE_SOCKET(server);

    WSACleanup();

    return success;

}