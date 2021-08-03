// Server.cpp: определяет точку входа для приложения.
//

#include "Server.h"



class ThreadPool {
private:

    std::condition_variable_any workQueueConditionVariable;
    std::vector<std::thread> threads;
    std::mutex workQueueMutex;
    std::queue<std::pair<int, std::string>> workQueue;
    bool done;

    void doWork() {
        while (!done) {
            std::pair<int, std::string> request;
            {
                std::unique_lock<std::mutex> g(workQueueMutex);
                workQueueConditionVariable.wait(g, [&] {
                    return !workQueue.empty() || done;
                    });

                request = workQueue.front();
                workQueue.pop();
            } close(request.first);
        }
    }

public:
    ThreadPool() : done(false) {
        auto numberOfThreads = std::thread::hardware_concurrency();
        if (numberOfThreads == 0) {
            numberOfThreads = 1;
        }for (unsigned i = 0; i < numberOfThreads; ++i) {
            threads.push_back(std::thread(&ThreadPool::doWork, this));
        }
    }

    ~ThreadPool() {
        done = true;

        workQueueConditionVariable.notify_all();
        for (auto& thread : threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    }


    void queueWork(int fd, std::string& request) {
        std::lock_guard<std::mutex> g(workQueueMutex);
        workQueue.push(std::pair<int, std::string>(fd, request));
        workQueueConditionVariable.notify_one();
    }
};

class Server {
private:
    int numPort;
    int sockfd;
    sockaddr_in sockaddr;

public:
    Server(int port) : numPort(port){};

    ~Server() {
        closeConn();
    };

    void openConn() {
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd == 0) {
            std::cout << "Failed to create socket. errno: " << errno << std::endl;
            exit(EXIT_FAILURE);
        }
        sockaddr.sin_family = AF_INET;
        sockaddr.sin_addr.s_addr = INADDR_ANY;
        sockaddr.sin_port = htons(numPort);
        if (bind(sockfd, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) < 0) {
            std::cout << "Failed to bind to port. errno: " << errno << std::endl;
            exit(EXIT_FAILURE);
        }
    }


    void listenClient(){
        if (listen(sockfd, 10) < 0) {
            std::cout << "Failed to listen on socket. errno: " << errno << std::endl;
            exit(EXIT_FAILURE);
        }
        ThreadPool tp;

        while (true) {
            auto addrlen = sizeof(sockaddr);
            int connection = accept(sockfd, (struct sockaddr*)&sockaddr, (socklen_t*)&addrlen);
            if (connection < 0) {
                std::cout << "Failed to grab connection. errno: " << errno << std::endl;
                exit(EXIT_FAILURE);
            }

            char* recvData = new char[100];
            int bytes = recv(connection, recvData, 99, 0);
            if (bytes == -1) {
                std::cout << "error on recv" << std::endl;
            }
            else {
                recvData[bytes] = '\0';
                std::cout << recvData << std::endl;
            }
            std::string request = recvData;
            writeFile(request);
            tp.queueWork(connection, request);
        }
    }
    

    void closeConn(){
        close(sockfd);
    }


    void writeFile(std::string buf) {
        std::string request = buf;
        std::ofstream out;
        out.open("\log.txt", std::ios::app);
        out << request << std::endl;
    }
};


int main(int argc, char* argv[]) {
    int port = atoi(argv[1]);
    Server s = {port};
    while (true){
        s.openConn();
        s.listenClient();
        s.closeConn();
    }
}