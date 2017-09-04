#include <iostream>
#include <string.h>
#include <fstream>
#include <vector>
#include <sys/stat.h>
#include <pthread.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sstream>
#include <iterator>

#include <UrlParser.h>


#define PORT 8081
#define LENGTH 512
#define THREADS 10

bool shouldRun = true;



template<typename Out>
void split(const std::string &s, char delim, Out result) {
    std::stringstream ss;
    ss.str(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        *(result++) = item;
    }
}


std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, std::back_inserter(elems));
    return elems;
}

struct sArgs
{
	unsigned short 	int id;
	int sockfd;
};

int SetupSockets(int &sockfd)
{
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == 0)
	{
		return -1;
	}

	int yes = 1;

	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)))
	{
		return -2;
	}

	struct sockaddr_in address;

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons( PORT );

	if (bind(sockfd, (struct sockaddr *)&address, sizeof(address)) < 0)
	{
		return -3;
	}

	if (listen(sockfd, 3) < 0)
	{
		return -4;
	}

	return 0;
}

std::string evaluatePath(std::string epath, std::string &code, std::string &connection, std::string &content_type)
{
	std::string path = UriDecode(epath);
	std::string rpath;
	std::string extension;

	if (path.find("..") != std::string::npos)
	{
		code = (std::string)"403 Forbidden";
		return "";
	}
	
	rpath = "/home/main/C++_Projects/Http/bin/" + path;

	if (rpath.at(rpath.length() - 1) == '/')
	{
		rpath += "index.html";
	}
	else
	{
		struct stat buffer;
		if (stat(rpath.c_str(), &buffer) == 0 && buffer.st_mode & S_IFREG)
		{
			connection = "Closed";
			code = "200 OK";
		}
		else if (stat((rpath + ".html").c_str(), &buffer) == 0)
		{
			rpath += ".html";

		}
		else if (stat((rpath + "/index.html").c_str(), &buffer) == 0)
		{
			rpath += "/index.html";
		}
	}

	extension = rpath.substr(rpath.find_last_of(".") + 1);
	if (extension == "html")
	{
		content_type = "text/html";
		connection = "Keep-Alive";
	}
	else if (extension == "css")
	{
		content_type = "text/css";
		connection = "Keep-Alive";
	}
	else if (extension == "png")
	{
		content_type = "image/png";
		connection = "Keep-Alive";
	}
	else if (extension == "jpg")
	{
		content_type = "image/jpeg";
		connection = "Keep-Alive";
	}
	else if (extension == "ico")
	{
		content_type = "image/x-icon";
		connection = "Keep-Alive";
	}
	else if (extension == "ttf")
	{
		content_type = "application/x-font-ttf";
		connection = "Keep-Alive";
	}
	else if (extension == "mp3")
	{
		content_type = "audio/mpeg";
		connection = "Keep-Alive";
	}
	else
	{
		content_type = "application/octet-stream";
		connection = "Keep-Alive";
	}


	return rpath;
}

std::ifstream::pos_type filesize(const char* filename)
{
    std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
    return in.tellg(); 
}


void* session(void *vsessionArguments)
{
	struct sArgs *sessionArguments;
	sessionArguments = (struct sArgs *)vsessionArguments;

	bool keepAlive;

	while (shouldRun == true)
	{
		int active_socket;
		if (keepAlive == false)
		{
			active_socket = accept(sessionArguments->sockfd, 0, 0);
		}
		if (active_socket != -1)
		{	
			char buffer[1024] = {0};
			std::string code;
			std::string content;
			std::string header;
			std::string responce;
			std::string connection;
			std::string path;
			std::string content_type;
			std::string content_length;

			read(active_socket, buffer, 1024);

			//std::cout << buffer << std::endl;

			std::vector<std::string> splitRequest = split(buffer, ' ');
				
			path = evaluatePath(splitRequest.at(1), code, connection, content_type);

			if (content_type == "text/html" || content_type == "text/css")
			{
				std::fstream sinfile;
				sinfile.open(path);
				if (sinfile.is_open())
				{
					while (!sinfile.eof()) // To get you all the lines.
					{
						std::string obj_l;

						getline(sinfile, obj_l); // Saves the line in STRING.

						content += obj_l;
					}
				}
				else
				{
					code = "404 Not Found";
					connection = "Closed";
					content_length = "0";
				}
			}

			content_length = std::to_string(filesize(path.c_str()));

			header = "HTTP/1.1 " + code +"\nContent-Lenght: " + content_length + "\nContent-Type: " + content_type  + "\nConnection: " + connection + "\n\r\n";

			responce = header + content;

			send(active_socket, responce.c_str(), responce.length(), 0);

			int ci;

			if (content_type == "image/jpeg" || content_type == "image/png" || content_type == "application/x-font-ttf" || content_type == "application/octet-stream" || content_type == "audio/mpeg")
			{
				FILE *fs = fopen(path.c_str(), "r");
			    if(fs == NULL)
			    {
			        code = "404 Not Found";
			    }
			    else
			    {
			    	code = "200 OK";
			    }

			    char sdbuf[LENGTH];

				bzero(sdbuf, LENGTH); 
			    int fs_block_sz;
			    while((fs_block_sz = fread(sdbuf, sizeof(char), LENGTH, fs)) > 0)
			    {
			        if(send(active_socket, sdbuf, fs_block_sz, 0) < 0)
			        {

			        }
			        ci++;
			        bzero(sdbuf, LENGTH);
			    }
			}

			std::cout << content_length <<  "-----" << splitRequest.at(1) << std::endl;

			if (connection == "Keep-Alive")
			{
				keepAlive = true;
			}
			else if (connection == "Closed")
			{
				keepAlive = false;
			}

			if (keepAlive == false)
			{
				shutdown(active_socket, SHUT_RDWR);
				int bempty;
				do {
					bempty = read(active_socket, buffer, 1024);
				} while (bempty != 0);
				close(active_socket);
			}
		}
	}

	pthread_exit(NULL);
}

int main()
{
	int sockfd;

	int socerr = SetupSockets(sockfd);

	if (socerr < 0)
	{
		return socerr;
	}

	pthread_t sessionThreads[THREADS];

	struct sArgs sessionArguments[THREADS];

	for (unsigned short int i; i < THREADS; i++)
	{
		sessionArguments[i].id = i;
		sessionArguments[i].sockfd = sockfd;

		int status = pthread_create(&sessionThreads[i], NULL, session, (void *)&sessionArguments[i]);
	}

	std::cout << "Press enter to exit: ";
	char c;
	std::cin >> c;
	std::cout << std::endl;

	close(sockfd);

	return 0;
}


  //   while (shouldRun == true)
  //   {
  //   	int active_socket = accept(*sockfd, 0, 0);
		// if (active_socket != -1)
		// {
		// 	vecThrd.push_back(active_socket);

		// 	sArgs sessionArguments;
	 //    	sessionArguments.vecThrd = &vecThrd;
	 //    	sessionArguments.id = vecThrd.size() - 1;
	 //    	sessionArguments.active_socket = active_socket;

	 //    	int session_thread = pthread_create(&vecThrd.at(vecThrd.size() - 1), NULL, session, (void *)&sessionArguments);
		// }
  //   }