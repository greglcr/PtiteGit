#include "hashing.h"
#include "object.h"
#include "object_commit.h"
#include "object_tree.h"
#include "object_file.h"
#include "reftag.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <vector>

Commit::Commit(Tree parentTree, std::vector<std::string> parentCommitsHash, std::string commitAuthor, std::string committer, std::string message, std::string gpgsig){
    this->commitAuthor = commitAuthor;
    this->committer = committer;
    this->parentCommitsHash = parentCommitsHash;
    this->parentTree = parentTree;
    this->gpgsig = gpgsig;
    this->message = message;

    std::string abcabc = "tree " + this->parentTree.getHashedContent() + "\n";
    for(long long ii = 0; ii < (long long) this->parentCommitsHash.size(); ii++) abcabc += ("parent " + this->parentCommitsHash[ii] + "\n" );
    abcabc += ("author " + this->commitAuthor + "\ncommitter " + this->committer + "\ngpgsig " + this->gpgsig + "\n" + this->message + "\n");
    
    long long uu = abcabc.size();
    this->content = "commit " + std::to_string(uu) + "\n" + abcabc;
    this->hashedContent = hashString(this->content);
}

void Commit::calculateContent(){
    std::string abcabc = "tree " + this->parentTree.getHashedContent() + "\n";
    for(long long ii = 0; ii < (long long) this->parentCommitsHash.size(); ii++) abcabc += ("parent " + this->parentCommitsHash[ii] + "\n" );
    abcabc += ("author " + this->commitAuthor + "\ncommitter " + this->committer + "\ngpgsig " + this->gpgsig + "\n" + this->message + "\n");
    
    long long uu = abcabc.size();
    this->content = "commit " + std::to_string(uu) + "\n" + abcabc;
    this->hashedContent = hashString(this->content);
}

Commit Commit::fromfile(std::string hashedContent){
    const std::ifstream input_stream(PtitGitRepos().getWorkingFolder() / ".ptitgit" / "objects" / get_path_to_object(hashedContent), std::ios_base::binary);

    if (input_stream.fail()) {
        throw std::runtime_error("Failed to open file");
    }

    std::stringstream buffer;
    buffer << input_stream.rdbuf();

    long long abcd = buffer.str().find(' ');
    long long dcba = buffer.str().find('\n');

    if(buffer.str().substr(0,abcd) != "commit") std::cerr<<"Not a commit!";
    if((long long) stoi(buffer.str().substr(abcd+1,dcba-abcd-1)) != (long long) buffer.str().size()-dcba-1) std::cerr<<"Bad size!";

    return Commit::fromstring(buffer.str().substr(dcba+1));
}

Commit Commit::fromstring(std::string commitContent){
    Commit X = Commit();
    long long abc = commitContent.find(' ');
    long long cba = commitContent.find('\n');
    long long mm = cba + 1;
    //if(commitContent.substr(mm,abc-mm) != "tree") std::cerr<<"Not a tree :(\n"<<commitContent.substr(mm,abc-mm)<<"\n";

    X.parentTree = findTree(commitContent.substr(abc+1, cba-abc-1),false);
    mm = cba+1;
    abc = commitContent.find(' ',mm);cba = commitContent.find('\n',mm);
    while(commitContent.substr(mm,abc-mm)=="parent"){
        std::string xyz = commitContent.substr(abc+1,cba-abc-1);
        X.parentCommitsHash.push_back(xyz);
        mm = cba+1;abc = commitContent.find(' ',mm);cba = commitContent.find('\n',mm);
    }
    X.commitAuthor = commitContent.substr(abc+1,cba-abc-1);
    mm = cba+1;abc = commitContent.find(' ',mm);cba = commitContent.find('\n',mm);
    X.committer = commitContent.substr(abc+1,cba-abc-1);
    mm = cba+1;abc = commitContent.find(' ',mm);cba = commitContent.find('\n',mm);
    X.gpgsig = commitContent.substr(abc+1,cba-abc-1);
    mm = cba+1;cba = commitContent.find('\n',mm);
    X.message = commitContent.substr(mm,cba-mm);
    X.calculateContent();
    return X;
}

Tree Commit::getTree(){
    return this->parentTree;
}
/*
void Commit::writeCommit(){
    fs::path curPath = fs::current_path();
    while (curPath != curPath.root_directory() && !fs::exists(curPath / ".ptitgit")) {
        curPath = curPath.parent_path();
    }
    fs::path path = curPath / ".ptitgit" / "objects" / this->getPathToWrite();
    fs::create_directory(curPath / ".ptitgit" / "objects" / std::string(this->getPathToWrite()).substr(0,2));
    std::ofstream out(path);
    out << this->content;
    return;
}*/

void checkout(std::string committ, fs::path placeToWrite, bool force){
    std::string commit = objectFind(PtitGitRepos(), committ, true, "commit");
    Commit C = Commit().fromfile(commit);
    Tree T = C.getTree();
    tree_checkout(T,placeToWrite,force);
}

void tree_checkout(Tree T, fs::path placeToWrite, bool force){
    if(!fs::exists(placeToWrite)) fs::create_directory(placeToWrite);
    if(!fs::is_empty(placeToWrite) && force == false) std::cerr<<"The directory given is not empty\n";
    std::vector<File> V = T.get_blobs_inside();
    for(long long kk = 0; kk < (long long) V.size(); kk++){
        fs::path path = V[kk].get_file_path().parent_path().filename();
        std::ofstream out(placeToWrite / path);
        out << V[kk].getContent();
    }
    
    for(std::vector<Tree>::iterator it = T.get_trees_inside().begin(); it!=T.get_trees_inside().end(); ++it)
        tree_checkout(*it,placeToWrite / it->get_folder_path().parent_path().filename(), force);
}

std::string Commit::get_hash_parent_tree() {

    return this->parentTree.getHashedContent();

}