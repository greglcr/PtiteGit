#include "init.h"
#include "../lib/object_commit.h"
#include "../lib/object_tree.h"
#include "../lib/object.h"

#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>

/* In order to handle file manipulation, we use the filesystem library introduced in C++17*/

namespace fs = std::filesystem;

void init() {

    init(fs::current_path());

}

void init(fs::path folderToInit) {
    

    if (folderToInit.is_relative()) {
        folderToInit = fs::current_path() / folderToInit;
    }

    if(fs::exists(folderToInit / ".ptitgit")) {
        std::cerr << "The given directory is already a git repos" << std::endl;
        return;
    }
    try {
        for(size_t i = 0; i < FOLDERS_TO_CREATE.size(); i++) {
            fs::create_directories(folderToInit / FOLDERS_TO_CREATE[i]);
        }
    }
    catch (const fs::filesystem_error& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    Tree T = Tree(folderToInit, true, true);
    Commit C = Commit(T, {}, "PtiteGit team", "PtiteGit team", "First commit");
    PtitGitRepos curRepos = PtitGitRepos(folderToInit);
    curRepos.writeObject(C);

    fs::path path = folderToInit / ".ptitgit" / "refs" / "heads" / "main";
    std::ofstream out(path);
    out << C.getHashedContent();

    fs::path headPath = folderToInit / ".ptitgit" / "HEAD";
    std::ofstream outHead(headPath);
    std::string indirectPath = "ref: " + std::string(relativeToRepo(path));
    outHead << indirectPath;

    std::ofstream config(folderToInit / ".ptitgit/config");
    config << "# this is the configuration file\n# it contains global associations key=value\n";
    config.close();

    INDEXreset(C, folderToInit);

}
