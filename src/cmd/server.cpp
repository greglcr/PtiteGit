#include "server.h"
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstdlib>
#include <vector>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "../lib/tcp.h"
#include "../cmd/init.h"
#include "../lib/compare_repos.h"
#include "../lib/reftag.h"

void* handle_tcp_connection (void* arg);
void server(int port)    {

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        std::cerr << "unable to create a socket" << std::endl;
        exit(EXIT_FAILURE);
    }

    sockaddr_in sockaddr;
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_addr.s_addr = INADDR_ANY;
    sockaddr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) < 0) {
        std::cerr << "Failed to bind  : " << errno << std::endl;
        exit(EXIT_FAILURE);
    }

    std::cout << "server started on port " << port << std::endl;

    if (listen(sockfd, 10) < 0) {
        std::cerr << "Failed to listen on socket. errno: " << errno << std::endl;
        exit(EXIT_FAILURE);
    }

    auto addrlen = sizeof(sockaddr);
    int accepted;
    std::vector<pthread_t> threads;
    while ((accepted = accept(sockfd,(struct sockaddr *)&sockaddr, (socklen_t*)&addrlen)) > 0) {
        threads.push_back(0);
        if( pthread_create(&(threads.back()), NULL, &handle_tcp_connection, (void*)&accepted) != 0){
            std::cerr << "Unable to create a new thread" << std::endl;
        }
        std::cout << threads.front() << " " << threads.back() << std::endl;
    }
}


void* handle_tcp_connection (void* arg)   {
    int* intPtr = (int*) arg;
    int connection = *intPtr;

    std::string cmd = read_message(connection, 100);

    if (cmd == "push")  {
        std::string repos_id = read_message(connection, 20);
        std::string directory = receive_repos(connection);
        if (!copy_all_objects(directory, repos_id)) {
            send_message(connection, "An error as occured, try again.");
            pthread_exit(EXIT_SUCCESS);
        }

        // copy all branches that are not in the remote repos
        std::map< fs::path, std::string > tags_local = ref_list_basic(PtitGitRepos(directory));
        for (auto it = tags_local.begin(); it != tags_local.end(); it++)  {
            std::string path = it->first;
            if (path.length() < 7) continue;
            if (path.back() == '/') path.pop_back();

            int posLast = path.rfind('/');
            if (posLast == (int)std::string::npos) continue;
            if ((int)path.rfind("heads", posLast-1) == posLast-5)    {
                std::string branchName = path.substr(posLast+1);
                if ( access((PtitGitRepos(repos_id).getWorkingFolder() / ".ptitgit" / "refs" / "heads" / branchName).c_str(), F_OK) == -1)    {
                    writeBranch(branchName, it->second, PtitGitRepos(repos_id));
                }
            }
        }
    


        result_compare branches = compare_branch(PtitGitRepos(directory), PtitGitRepos(repos_id));
        if (branches.branch_name == "") {
            send_message(connection, "You are not on a branch, you can not push");
        } else if (branches.commit_B == "") {
            send_message(connection,"this branch doesn't exist on the remote repository. Initializing....");
            writeBranch(branches.branch_name, branches.commit_A, PtitGitRepos(repos_id));
        } else if (branches.commit_A == branches.commit_B)  {
            send_message(connection, "nothing to push. This branch is already synchronized with the server.");
        } else if (branches.commit_A == branches.commit_lca)    {
            send_message(connection, "Nothing to push. All your commits are already on the server.\nBe careful, the server is a few commits ahead.");
        } else if (branches.commit_B == branches.commit_lca)    {
            writeBranch(branches.branch_name, branches.commit_A, PtitGitRepos(repos_id));
            send_message(connection, "Well received !");
        } else  {
            send_message(connection, "'push' impossible. Your branches diverge.");
        }
        std::filesystem::remove_all(directory);
    } else if (cmd == "pull")   {
        std::string repos_id = read_message(connection, 200);
        send_repos(connection, repos_id);
    } else if (cmd == "init-remote")    {
        long long repos_id = 1;
        struct stat info;
        while (stat(std::to_string(repos_id).c_str(), &info) == 0)
            repos_id++;
        
        std::string name_folder = std::to_string(repos_id);
        mkdir(name_folder.c_str(), 0700);
        std::cout << "Create repos with id : " << repos_id << std::endl;
        init(name_folder);
        send_message(connection,"repos created with id " + name_folder);
    } else  {
        send_message(connection,"INVALID ACTION");
    }
    pthread_exit(EXIT_SUCCESS);
}
