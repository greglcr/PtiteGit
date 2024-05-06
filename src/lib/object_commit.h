#ifndef OBJECT_COMMIT_H
#define OBJECT_COMMIT_H

#include "object.h"
#include "repos.h"
#include "object_tree.h"

#include <vector>

class Commit : public Object {

    public:

        Commit(std::vector<Commit> parentCommits = {}, Tree parentTree, std::string commitAuthor = "", std::string committer ="PtiteGit Team", std::string message="New commit", std::string gpgsig="");

    private:

        std::vector<Commit> parentCommits;
        std::string commitAuthor;
        std::string committer;
        std::string message;
        std::string gpgsig;
        Tree parentTree;
        //PtitGitRepos repos;

};

#endif