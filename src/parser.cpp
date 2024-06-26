#include "cmd/add.h"
#include "cmd/cat-object.h"
#include "cmd/init.h"
#include "cmd/hash_object.h"
#include "cmd/server.h"
#include "cmd/help.h"
#include "cmd/push.h"
#include "cmd/pull.h"
#include "cmd/status.h"
#include "cmd/commit.h"
#include "cmd/merge.h"
#include "cmd/init-remote.h"
#include "lib/hashing.h"
#include "lib/object.h"
#include "lib/object_file.h"
#include "lib/object_tree.h"
#include "lib/repos.h"
#include "lib/reftag.h"

#include <iostream>
#include <string.h>
#include <map>
#include <vector>

int edit_distance(std::string a, std::string b) {
    if (a == b) return 0;

    int n = a.length();
    int m = b.length();
    if (n == 0)  return m;
    if (m == 0)  return n;

    std::string aBis = a.substr(0, n-1);
    std::string bBis = b.substr(0, m-1);

    if (a[n-1] == b[m-1])   return edit_distance(aBis, bBis);
    return 1 + std::min( edit_distance(aBis, bBis), std::min(edit_distance(aBis, b), edit_distance(a, bBis)));
}



int main(int argc,char *argv[])  {

    if (argc > 1 && strcmp(argv[1],"init") == 0) {
        if (argc > 2)
            init(argv[2]);
        else
            init();
    }
    else if(argc > 2 && strcmp(argv[1], "hash-object") == 0) {
        if (argc < 3)
            std::cerr << "Error : Object to hash missing" << std::endl;
        else
            std::cout << hash_object(File(argv[2])) << std::endl; 
    }
    else if (argc > 2 && strcmp(argv[1], "cat-file") == 0) {
        if(argc < 3)
            std::cerr << "Error : Object to display missing" << std::endl;
        else
            std::cout << cat_object(argv[2]) << std::endl;
    }
    else if (argc > 1 && strcmp(argv[1], "server") == 0) {
        if (argc >= 3)
            server(atoi(argv[2]));
        else
            server();
    }
    else if (argc > 1 && strcmp(argv[1], "help") == 0) {
        help();
    }
    else if (argc > 1 && strcmp(argv[1] , "show-ref") == 0){
        PtitGitRepos X = PtitGitRepos();
        std::map <fs::path, std::string> Y = ref_list_basic(X);
        for (const auto &[k, v] : Y) std::cout<<"The ref in "<<k<<" is "<<v<<".\n";
    }
    else if (argc > 1 && strcmp(argv[1] , "tag") == 0){
        long long k;bool create;PtitGitRepos X = PtitGitRepos();std::string sha, content;Tag xyz = Tag();
        if(argc == 2){
            std::map <fs::path, std::string> Y = ref_list({}, X, X.getWorkingFolder() / ".ptitgit" / "refs" / "heads");
            for (const auto &[k, v] : Y) std::cout<<"The branch "<<k<<" points to commit of hash "<<v<<".\n";
        }
        else if(strcmp(argv[2] , "-a") == 0){create = true; k = 3;}
        else{create = false; k = 2;}
        if(argc < k + 1) std::cerr<<"Ref name missing"<<std::endl;
        else if(argc == k + 2){
            if(fs::exists(X.getWorkingFolder() / ".ptitgit" / "refs" / "tags" / argv[k+1])){
                sha = ref_resolve(X, X.getWorkingFolder() / ".ptitgit" / "refs" / "tags" / argv[k+1]);
                xyz.tag_create(X, argv[k], sha, "???", create);
            }
            else if(fs::exists(X.getWorkingFolder() / ".ptitgit" / "refs" / "heads" / argv[k+1])){
                sha = ref_resolve(X, X.getWorkingFolder() / ".ptitgit" / "refs" / "heads" / argv[k+1]);
                xyz.tag_create(X, argv[k], sha, "???", create);
            }
            else xyz.tag_create(X, argv[k], argv[k+1], "????", create);
        }
        else if(argc == k + 1){
            if(fs::exists(X.getWorkingFolder() / ".ptitgit" / "HEAD")){
                sha = ref_resolve(X, X.getWorkingFolder() / ".ptitgit" / "HEAD");
                xyz.tag_create(X, argv[k], sha, "???", create);
            }
            else std::cerr<<"Object missing!"<<std::endl;
        }
    }
    else if (argc > 1 && strcmp(argv[1] , "branch") == 0){
        PtitGitRepos X = PtitGitRepos();std::string sha;
        if(argc == 2) std::cerr<<"Name missing!"<<std::endl;
        else{
            sha = ref_resolve(X, X.getWorkingFolder() / ".ptitgit" / "HEAD");
            writeBranch(argv[2], sha);
        }
    }
    else if (argc > 1 && strcmp(argv[1] , "checkout") == 0){
        PtitGitRepos X = PtitGitRepos();
        if(argc < 4) std::cerr<<"Something missing!\n";
        else if(strcmp(argv[2] , "-f") == 0 && argc == 4) checkout(argv[3], X.getWorkingFolder() , true);
        else if(strcmp(argv[2] , "-f") == 0) checkout(argv[3], argv[4], true);
        else checkout(argv[2], argv[3]);
    }
    else if (argc > 1 && strcmp(argv[1] , "commit") == 0){
        if(argc > 3 && strcmp(argv[2] , "-m") == 0) cmdCommit(argv[3]);
        else cmdCommit();
    }
    else if (argc > 1 && strcmp(argv[1] , "merge") == 0){
        if(argc == 2) std::cerr<<"Where is the other commit?\n";
        else if(argc == 3) merge(argv[2]);
        else merge(argv[2],argv[3]);
        std::cout<<"Merge sucessfully\n";
    }
    else if (argc >= 2 && strcmp(argv[1] , "push") == 0){
        push();
    }
    else if (argc >= 2 && strcmp(argv[1] , "pull") == 0){
        pull();
    }

    else if (argc >= 2 && strcmp(argv[1] , "config") == 0){
        PtitGitRepos X = PtitGitRepos();
        if (argc == 3)
            std::cout << X.get_config(argv[2]) << std::endl;
        else if (argc == 4)
            X.set_config(argv[2], argv[3]);
        else
            std::cout << "this command takes 1 or 2 arguments (to show a config value or set a config value)" << std::endl;
    }
    else if (argc >= 2 && strcmp(argv[1], "status") == 0) {
        if (argc >= 3) {
            status(argv[2]);
        }
        else {
            status();
        }
    }
    else if (argc >= 2 && strcmp(argv[1], "add") == 0) {
        if (argc >= 3) {
            add(argv[2]);
        }
        else {
            add();
        }
    }
    else if (argc >= 2 && strcmp(argv[1], "init-remote") == 0) {
        if (argc >= 4)
            init_remote(argv[2], atoi(argv[3]));
        else
            std::cout << "Please specify url and port" << std::endl;
    }


    else if (argc >= 2)   {
        std::cerr << "'" << argv[1] << "' is not a command." << std::endl;
        std::vector<std::string> lst_commands { "help","init", "hash-object", "cat-file", "server", "show-ref", "tag", "branch", "checkout", "commit", "merge", "push", "pull", "config", "status", "add","init-remote"};


        int dist_min = 100000000;
        std::string command_min = "???????";
        for (auto cmd : lst_commands)   {
            int dist = edit_distance(cmd, argv[1]);
            if (dist <= dist_min)   {
                dist_min = dist;
                command_min = cmd;
            }
        }
        std::cerr << "Maybe you want to use :    " << command_min << std::endl;
    }
    else {
        help();
    }
}
    
