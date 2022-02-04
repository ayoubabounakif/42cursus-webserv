/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   socketsIO.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: abiari <abiari@student.1337.ma>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/12/24 10:41:08 by abiari            #+#    #+#             */
/*   Updated: 2022/02/04 11:58:33 by abiari           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "socketsIO.hpp"

socketsIO::socketsIO(): _nfds(0), _socksAlloc(), _socks(), _pollfds() {}

socketsIO::socketsIO(const socketsIO& x): _nfds(0) { this->operator=(x); }

socketsIO::~socketsIO(){
	std::vector<sockets *>::iterator it = _socks.begin();
	std::vector<sockets *>::iterator ite = _socks.end();
	while (it != ite){
		_socksAlloc.destroy(*it);
		_socksAlloc.deallocate(*it, 1);
		it++;
	}
}

socketsIO&	socketsIO::operator=(const socketsIO& x){
	_nfds = x._nfds;
	_socksAlloc = x._socksAlloc;
	_socks = x._socks;
	_pollfds = x._pollfds;
	return *this;
}

void	socketsIO::setSock(const sockets& sock){
	struct	pollfd	fds = {};
	sockets *newSock = _socksAlloc.allocate(1);
	_socksAlloc.construct(newSock, sock);
	std::vector<int>::iterator it = newSock->getClientsVec().begin();
	std::vector<int>::iterator ite = newSock->getClientsVec().end();
	_socks.push_back(newSock);
	fds.fd = newSock->getMainSock();
	fds.events = POLLIN;
	_pollfds.push_back(fds);
	_nfds++;
	while (it != ite){
		fds.fd = *it;
		fds.events = POLLIN | POLLOUT;
		_pollfds.push_back(fds);
		_nfds++;
		it++;
	}
}

void	socketsIO::eventListener(){
	char				buffer[1024];
	std::string			req;
	struct	pollfd	fds = {};
	int				wasMainSock;
	int		rc;
	bool	endServer = false/* , closeConn = false */;
	bool	connClosed = false;
	while(!endServer) {
		wasMainSock = 0;
		std::cout << "Waiting on poll..." << std::endl;
		rc = poll(&_pollfds[0], _nfds, -1);
		if (rc < 0)
			throw socketIOErr("poll: ");
		if (rc == 0)
			throw socketIOErr("poll: ");
		for (int i = 0; i < _nfds; i++) {
			if (_pollfds[i].revents == 0)
				continue;
			if (_pollfds[i].revents != POLLIN && _pollfds[i].revents != POLLOUT && _pollfds[i].revents != (POLLIN | POLLOUT))
			{
				std::cout << "Error: revents = " << std::hex <<_pollfds[i].revents << std::endl;
				endServer = true;
				break;
			}
			for(size_t j = 0; j < _socks.size(); j++){
				if(_pollfds[i].fd != _socks[j]->getMainSock())
					continue ;
				wasMainSock = 1;
				std::cout << "socket listening on port: " << _socks[j]->getPort() << " is readable" << std::endl;
				try {
					fds.fd = _socks[j]->acceptClient();
					fds.events = POLLIN | POLLOUT;
					_nfds++;
					_pollfds.push_back(fds);
				}
				catch(const std::exception& e) {
					std::cerr << e.what() << '\n';
					endServer = true;
					break ;
				}
			}
			if(!wasMainSock){
				bzero(buffer, 1024);
				do
				{
					rc = recv(_pollfds[i].fd, &buffer, sizeof(buffer), 0);
					req.append(buffer);
					if (rc == -1)
						break ;
					if(rc == 0)
						connClosed = true;
					std::cout << "received: " << rc << "bytes" << std::endl;
					std::cout << "===============REQUEST BEGIN===================\n";
					std::cout << req << std::endl;
					std::string res("HTTP/1.1 200 OK\nConnection: close\nContent-Type: text/html\nContent-Length: 304\n\n<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"https://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n<html xmlns=\"http://www.w3.org/1999/xhtml\">\n<head>\n<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />\n<title>Webserv</title>\n<body>\n<h1>Connected to server</h1>\n</body>\n</html>");
					send(_pollfds[i].fd, res.c_str(), res.length(), 0);
					connClosed = true;
				} while (!connClosed);
				if (connClosed) {
					close(_pollfds[i].fd);
					_pollfds.erase(_pollfds.begin() + i);
					_nfds--;
					connClosed = false;
				}
				
				// _requests[_pollfds[i].fd].append(&buffer[0]);
				
			}
		}
	}
}
