#include "../lib/object.h"
#include "../lib/object_commit.h"
#include "../lib/object_tree.h"
#include "../lib/repos.h"
#include "../lib/reftag.h"
#include "commit.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>

void cmdCommit(std::string message){
    PtitGitRepos X = PtitGitRepos();
    fs::path path = "index";
    path = path / "INDEX";

    std::string oldsha = X.get_repos_content(path);
    long long endlFind = oldsha.find('\n');
    Tree T = findTree(oldsha.substr(0,endlFind),true);
    std::string sha = ref_resolve(X,"HEAD");
    Commit New = Commit(T,{sha},"","",message,"");
    New.writeObject();

    std::string content = X.get_repos_content("HEAD");
    if(content[0] == 'r' && content[1] == 'e' && content[2] == 'f' && content[3] == ':' && content[4] == ' '){
        path = content.substr(5);
        remove(path);
        std::ofstream out(path);
        out << New.getHashedContent();
        return;        
    }
    else{
        remove(X.getWorkingFolder() / ".ptitgit" / "HEAD");
        std::ofstream out(X.getWorkingFolder() / ".ptitgit" / "HEAD");
        out << New.getHashedContent();
        return;
    }
}