#ifndef REFTAG_H
#define REFTAG_H

#include <map>
#include "repos.h"
#include "object.h"
#include "object_commit.h"
#include<iostream>
#include<vector>

namespace fs = std::filesystem;

std::string ref_resolve(PtitGitRepos X, fs::path path);
std::map <fs::path, std::string> ref_list_basic(PtitGitRepos X);
std::map <fs::path, std::string> ref_list(std::map <fs::path, std::string> current, PtitGitRepos X, fs::path path);

class Tag : public Object {
    public:
        Tag(Object tagObject = Commit(), std::string tagType = "commit", std::string tagAuthor = "", std::string tagger ="PtiteGit Team", std::string tagName="abc", std::string tagMessage = "New tag");
        void tag_create(PtitGitRepos X, std::string tag_name, std::string tagged_object, std::string tag_message, bool create=false);
        void calculateContent();
        Tag fromfile(std::string);
        Tag fromstring(std::string);
        Object getObject();
    private:
    Object tagObject;
    std::string tagAuthor;
    std::string tagger;
    std::string tagName;
    std::string tagMessage;
    std::string tagType;
};

void writeRef(std::string, std::string, PtitGitRepos repos = PtitGitRepos());
void writeBranch(std::string, std::string, PtitGitRepos repos = PtitGitRepos());

std::vector <std::string> objectResolve(PtitGitRepos,std::string,bool);
std::string objectFind(PtitGitRepos X,std::string name,bool short_hash = true, std::string type = "object",bool follow = true);
#endif
