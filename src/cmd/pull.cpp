#include "pull.h"
#include "../lib/repos.h"
#include "../lib/tcp.h"
#include "../lib/compare_repos.h"
#include "../lib/reftag.h"
#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <filesystem>
#include <fstream>

void pull() {

    PtitGitRepos repos = PtitGitRepos();

    std::string url = repos.get_config("url");
    if (url == "")  {
        std::cerr << "url of the server not available. Please add it with :\n    ptitGit config url your_url" << std::endl;
        return;
    }

    std::string repos_id = repos.get_config("repos-id");
    if (repos_id == "")  {
        std::cerr << "id of the repos not available. Please add it with :\n    ptitGit config repos-id your_id" << std::endl;
        return;
    }
    
    std::string port = repos.get_config("port");
    if (port == "")  {
        std::cerr << "port of the repos not available. Please add it with :\n    ptitGit config port your_port" << std::endl;
        return;
    }

    int sockfd = connect_to_server(url, stoi(port));
    send_message(sockfd, "pull");
    send_message(sockfd, repos_id);

    std::string tmp_dir = receive_repos(sockfd);
    if (tmp_dir == "")  {
        std::cout << "repository not received" << std::endl;
        close(sockfd);
        return;
    }
    close(sockfd);


    if (!copy_all_objects(tmp_dir, ".")) {
        std::cerr << "An error as occured, try again." << std::endl;
        return;
    }

    // copy all branches that are not in the local repos
    std::map< fs::path, std::string > tags_remote = ref_list_basic(PtitGitRepos(tmp_dir));
    for (auto it = tags_remote.begin(); it != tags_remote.end(); it++)  {
        std::string path = it->first;
        if (path.length() < 7) continue;
        if (path.back() == '/') path.pop_back();

        int posLast = path.rfind('/');
        if (posLast == (int)std::string::npos) continue;
        if ((int)path.rfind("heads", posLast-1) == posLast-5)    {
            std::string branchName = path.substr(posLast+1);
            if ( access((PtitGitRepos().getWorkingFolder() / ".ptitgit" / "refs" / "heads" / branchName).c_str(), F_OK) == -1)    {
                std::cout << "a new branch '" << branchName << "' was created" << std::endl;
                writeBranch(branchName, it->second);
            }
        }
    }
    

    // compare commits and ask for a merge if needed
    result_compare branches = compare_branch(PtitGitRepos(), PtitGitRepos(tmp_dir));

    if (branches.branch_name == "") {
        std::cout << "Warning, you are not working on a branch. Therefore the 'pull' did nothing" << std::endl;
    } else if (branches.commit_B == "") {
        std::cout << "Warning, your branch '" << branches.branch_name << "' does not exist in the remote repository. Therefore 'pull' did nothing" << std::endl;
    } else if (branches.commit_A == branches.commit_B)  {
        std::cout << "Your branch '"<<branches.branch_name << "'is up to date" << std::endl;
    } else if (branches.commit_B == branches.commit_lca)    {
        std::cout << "You're a few commits ahead of the server. Use 'pull' to send them." << std::endl;
    } else if (branches.commit_A == branches.commit_lca)    {
        std::cout << "Download and update (the remote repository was a few commits ahead) on branch '" << branches.branch_name << "'" << std::endl;
        writeBranch(branches.branch_name, branches.commit_B);
        //checkout(branches.commit_B, PtitGitRepos().getWorkingFolder(), true);
        checkout(branches.branch_name, PtitGitRepos().getWorkingFolder(), true);
    } else {
        std::cout << "MERGE CONFLICT on your branch '" << branches.branch_name << "'!\nlocal last commit :  " << branches.commit_A << "\nremote last commit : " << branches.commit_B << "\nlca commit :         " << branches.commit_lca << std::endl;
        std::cout << "please, merge them" << std::endl;
    }

    std::filesystem::remove_all(tmp_dir);
}
