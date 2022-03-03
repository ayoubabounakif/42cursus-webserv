/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigClass.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aabounak <aabounak@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/02/14 15:45:02 by aabounak          #+#    #+#             */
/*   Updated: 2022/03/03 14:40:37 by aabounak         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# include "ConfigClass.hpp"

/* ----- Constructors & Destructor respectively ----- */
ConfigClass::ConfigClass() : _configFile("./conf.d/default.conf"), _serverCount(0), _serverConf(0) {}
ConfigClass::ConfigClass( std::string const & configFile ) { this->_configFile = configFile; }
ConfigClass::ConfigClass( ConfigClass const &rhs ) { *this = rhs; }
ConfigClass & ConfigClass::operator =( ConfigClass const & rhs) {
    if (this != &rhs) {
        this->_configFile = rhs._configFile; 
        this->_serverCount = rhs._serverCount;
        /* -- {DEEP COPY} _serverConf */
        this->_serverConf = rhs._serverConf;
    }
    return *this;
}
ConfigClass::~ConfigClass() {}

/* ----- Getters ---- */
std::string 	ConfigClass::getConfigFile( void ) const { return this->_configFile; }
size_t			ConfigClass::getServerCount( void ) const { return this->_serverCount; }
std::vector<ServerConfigClass> ConfigClass::getServerConfigClass( void ) const { return _serverConf; }

/* ----- Setters ---- */
void    ConfigClass::_allocateServers( void ) {
    std::ifstream   file(this->_configFile);
    std::string     buffer;
    size_t   n = 0;
    while (getline(file, buffer)) {
        if (buffer.find("server {") != std::string::npos)
            n++;
    }
    this->_serverCount = n;
    for (size_t i = 0; i < this->_serverCount; i++)
        this->_serverConf.push_back(ServerConfigClass());+
}

void    ConfigClass::_allocateLocations( void ) {
    std::ifstream   file(this->_configFile);
    std::string     buffer;
    size_t   n_serv = 0;
    size_t   n_loc = 0;
    while (getline(file, buffer)) {
        if (buffer.find("server {") != std::string::npos) {
            while (getline(file, buffer)) {
                if (buffer.find("}") != std::string::npos) {
                    this->_serverConf[n_serv]._locationCount = n_loc;
                    for (size_t i = 0; i < n_loc; i++) {
                        this->_serverConf[n_serv]._location.push_back(LocationClass());
                    }
                    n_serv++;
                    n_loc = 0;
                    break ;
                }
                else if (buffer.find("location = [") != std::string::npos)
                    n_loc++;
            }
        }
    }
}

void    ConfigClass::_checkConfigValidity( void ) {
    
    return ;
}

/* ----- Main Parser ----- */

const std::string WHITESPACE = " ";

std::string ltrim(const std::string &s) {
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}

std::string rtrim(const std::string &s) {
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

std::string trim(const std::string &s) {
    return rtrim(ltrim(s));
}

void    ConfigClass::parseConfigFile( void ) {
    std::ifstream	file(this->_configFile);
    std::string		buffer;
    size_t			n_serv = 0; 
    this->_allocateServers();
    this->_allocateLocations();

    std::cout << "Server count = " << this->_serverCount << std::endl;

    while (getline(file, buffer)) {
        buffer = trim(buffer);
        if (buffer != "server {") {
            if ((buffer[0] == '#' && buffer.find_first_of("#") == 0) || buffer.empty()) continue ;
            else throw parseErr("SyntaxError || WEIRD AF");
        }
        else {
            size_t n_loc = 0;
            while (getline(file, buffer)) {
                buffer = trim(buffer);
                if (buffer == "}") break ;
                switch (buffer[0]) {
                    case 'a':
                        if (std::strncmp("access_log = ", buffer.c_str(), 13) == 0) {
                            this->_serverConf[n_serv]._accessLog = buffer.substr(buffer.find("access_log = ") + strlen("access_log = "));
                            break ;
                        }
                        else if (std::strncmp("autoindex = ", buffer.c_str(), 12) == 0) {
                            if (std::strncmp("autoindex = on", buffer.c_str(), 14) == 0) this->_serverConf[n_serv]._autoindex = _AUTOINDEX_ON_;
                            else if (std::strncmp("autoindex = off", buffer.c_str(), 15) == 0) this->_serverConf[n_serv]._autoindex = _AUTOINDEX_OFF_;
                            break ;
                        }
                        throw parseErr("SyntaxError || 1");
                    case 'b':
                        if (std::strncmp("body_size_limit = ", buffer.c_str(), 18) == 0) {
                            this->_serverConf[n_serv]._bodySizeLimit = std::stoi(buffer.substr(buffer.find("body_size_limit = ") + strlen("body_size_limit = ")));
                            break ;
                        }
                        throw parseErr("SyntaxError || 2");
                    case 'e':
                        if (std::strncmp("error_page = ", buffer.c_str(), 13) == 0) {
                            this->_serverConf[n_serv]._errorPage = buffer.substr(buffer.find("error_page = ") + strlen("error_page = "));
                            break ;
                        }
                        throw parseErr("SyntaxError || 3");
                    case 'l':
                        if (std::strncmp("listen = ", buffer.c_str(), 9) == 0) {
                            this->_serverConf[n_serv]._port = std::stoi(buffer.substr(buffer.find("listen = ") + strlen("listen = ")));
                            break ;
                        }
                        else if (std::strncmp("location = [", buffer.c_str(), 12) == 0) {
                            while (getline(file, buffer)) {
                                buffer = trim(buffer);
                                if (buffer == "]") break ;
                                this->_serverConf[n_serv]._location[n_loc].parseLocation(buffer);
                            }
                            n_loc++;
                            break ;
                        }
                        throw parseErr("SyntaxError || 4");
                    case 'r':
                        if (std::strncmp("root = ", buffer.c_str(), 7) == 0) {
                            this->_serverConf[n_serv]._root = buffer.substr(buffer.find("root = ") + strlen("root = "));
                            if (this->_serverConf[n_serv]._root[this->_serverConf[n_serv]._root.size() - 1] != '/')
                                this->_serverConf[n_serv]._root += "/";
                            break ;
                        }
                        else if (std::strncmp("redirect = ", buffer.c_str(), 11) == 0) {
                            this->_serverConf[n_serv]._redirect = buffer.substr(buffer.find("redirect = ") + strlen("redirect = "));
                            break ;
                        }
                        throw parseErr("SyntaxError || 5");                        
                    case 's':
                        if (std::strncmp("server_name = ", buffer.c_str(), 14) == 0) {
                            this->_serverConf[n_serv]._serverName = buffer.substr(buffer.find("server_name = ") + strlen("server_name = "));
                            break ;
                        }
                        throw parseErr("SyntaxError || 6");
                    default:
                        if (buffer.empty()) break ;
                        throw parseErr("SyntaxError || SERVER SIDE");
                }
            }
            n_serv++;
        }
    }
}
