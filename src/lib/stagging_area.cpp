#include "hashing.h"
#include "repos.h"
#include "stagging_area.h"
#include "object_tree.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;

bool NodeTree::operator<(const NodeTree &other) {
    return this->nodeHash < other.nodeHash;
}

void StaggingArea::construct_tree(std::string curFileHash) {


    std::string fileContent = this->repos.get_repos_content(fs::path("index") / get_path_to_object(curFileHash));

    long long nextSpace = fileContent.find(' ');
    std::string fileType = fileContent.substr(0,nextSpace);

    if (fileType == "tree") {
        try {
            get_next_line(fileContent);
            get_next_line(fileContent);
            while(fileContent != "") {
                std::string curLine = get_next_line(fileContent);
                std::string nextFileHash = get_object_hash(curLine);
                this->treeStaggingArea[curFileHash].push_back(nextFileHash);
                this->treeStaggingAreaReversed[nextFileHash] = curFileHash;
                construct_tree(nextFileHash);
            }
        }
        catch (const std::runtime_error &e) {
            //Nothing to do
        }
    }
    else if (fileType == "file") {
        //Nothing to do
    }
    else {
        std::cerr << "Error in StaggingArea::construct_tree : invalid file type (" << fileType << ")\n";
    }

}



void StaggingArea::calc_differences(bool verbose = false) {

    this->status[repos.getWorkingFolder()].second = repos.get_repos_content("index/INDEX");

    this->added.clear();
    this->deleted.clear();
    this->modified.clear();

    calc_differences(repos.getWorkingFolder(), repos.get_repos_content("index/INDEX"), verbose);

    if (verbose) {
        this->get_stagging_area();
    }

}

void StaggingArea::calc_differences(fs::path curPathInWorkingArea, std::string curHashStaggingArea, bool verbose) {

    Tree curTreeInWorkingArea = Tree(curPathInWorkingArea);

    std::string curContentStaggingArea = repos.get_repos_content(fs::path("index") / get_path_to_object(curHashStaggingArea));
    get_next_line(curContentStaggingArea);

    fs::path curPathStaggingArea = fs::path(get_next_line(curContentStaggingArea));
    curPathStaggingArea = fs::current_path() / curPathStaggingArea;
    std::string stringPath = curPathStaggingArea.string();
    if (stringPath.back() == '/') {
        stringPath.pop_back();
    }
    curPathStaggingArea = fs::path(stringPath);

    std::vector<File> blobsInCurWorkingFolder = curTreeInWorkingArea.get_blobs_inside();
    std::vector<std::pair<fs::path, std::string> > pathToBlobsInCurWorkingFolder;
    for (size_t i = 0; i < blobsInCurWorkingFolder.size(); i++) {
        pathToBlobsInCurWorkingFolder.push_back(std::pair(blobsInCurWorkingFolder[i].get_file_path(), blobsInCurWorkingFolder[i].getHashedContent()));
    }

    std::vector<Tree> treesInCurWorkingFolder = curTreeInWorkingArea.get_trees_inside();
    std::vector<std::pair<fs::path, std::string> > pathToTreesInCurWorkingFolder;
    for (size_t i = 0; i < treesInCurWorkingFolder.size(); i++) {
        pathToTreesInCurWorkingFolder.push_back(std::pair(treesInCurWorkingFolder[i].get_folder_path(), treesInCurWorkingFolder[i].getHashedContent()));
    }

    std::vector<std::pair<fs::path, std::string> > pathToBlobsInCurStaggingArea;
    std::vector<std::pair<fs::path, std::string> > pathToTreesInCurStaggingArea;
    while(curContentStaggingArea != "") {
        std::string curLine = get_next_line(curContentStaggingArea);        
        if (get_object_type(curLine) == "file") {
            pathToBlobsInCurStaggingArea.push_back(std::pair(curPathStaggingArea / get_object_path(curLine), get_object_hash(curLine)));
        }
        else {
            pathToTreesInCurStaggingArea.push_back(std::pair(curPathStaggingArea / get_object_path(curLine), get_object_hash(curLine)));
        }
    }

    //std::vector<std::string> deleted, added, modified;

    std::sort(pathToBlobsInCurWorkingFolder.begin(), pathToBlobsInCurWorkingFolder.end());
    std::sort(pathToBlobsInCurStaggingArea.begin(), pathToBlobsInCurStaggingArea.end());
    int posWorkingFolder = 0, posStaggingArea = 0;
    while (posWorkingFolder < (int)pathToBlobsInCurWorkingFolder.size() || posStaggingArea < (int)pathToBlobsInCurStaggingArea.size()) {
        if (posWorkingFolder == (int)pathToBlobsInCurWorkingFolder.size()) {
            //if(verbose){std::cout << "Fichier supprimé : " << relativeToRepo(pathToBlobsInCurStaggingArea[posStaggingArea].first) << std::endl;}
            this->deleted.push_back(pathToBlobsInCurStaggingArea[posStaggingArea].first);
            this->status[pathToBlobsInCurStaggingArea[posStaggingArea].first].first = "deleted";
            this->status[pathToBlobsInCurStaggingArea[posStaggingArea].first].second = pathToBlobsInCurStaggingArea[posStaggingArea].second;
            posStaggingArea++;
            
        }
        else if (posStaggingArea == (int)pathToBlobsInCurStaggingArea.size()) {
            //if(verbose){std::cout << "Fichier ajouté : " << relativeToRepo(pathToBlobsInCurWorkingFolder[posWorkingFolder].first) << std::endl;}
            this->added.push_back(pathToBlobsInCurWorkingFolder[posWorkingFolder].first);
            this->status[pathToBlobsInCurWorkingFolder[posWorkingFolder].first].first = "added";
            posWorkingFolder++;
        }
        else if (pathToBlobsInCurWorkingFolder[posWorkingFolder].first > pathToBlobsInCurStaggingArea[posStaggingArea].first) {
            //if(verbose){std::cout << "Fichier supprimé : " << relativeToRepo(pathToBlobsInCurStaggingArea[posStaggingArea].first) << std::endl;}
            this->deleted.push_back(pathToBlobsInCurStaggingArea[posStaggingArea].first);
            this->status[pathToBlobsInCurStaggingArea[posStaggingArea].first].first = "deleted";
            this->status[pathToBlobsInCurStaggingArea[posStaggingArea].first].second = pathToBlobsInCurStaggingArea[posStaggingArea].second;
            posStaggingArea++;
        }
        else if (pathToBlobsInCurStaggingArea[posStaggingArea].first > pathToBlobsInCurWorkingFolder[posWorkingFolder].first) {
            //if(verbose){std::cout << "Fichier ajouté : " << relativeToRepo(pathToBlobsInCurWorkingFolder[posWorkingFolder].first) << std::endl;}
            this->added.push_back(pathToBlobsInCurWorkingFolder[posWorkingFolder].first);
            this->status[pathToBlobsInCurWorkingFolder[posWorkingFolder].first].first = "added";
            posWorkingFolder++;
        }   
        else if (pathToBlobsInCurStaggingArea[posStaggingArea].second != pathToBlobsInCurWorkingFolder[posWorkingFolder].second) {
            //if(verbose){std::cout << "Fichier modifié : " << relativeToRepo(pathToBlobsInCurWorkingFolder[posWorkingFolder].first) << std::endl;}
            this->modified.push_back(pathToBlobsInCurWorkingFolder[posWorkingFolder].first);
            this->status[pathToBlobsInCurWorkingFolder[posWorkingFolder].first].first = "modified";
            this->status[pathToBlobsInCurWorkingFolder[posWorkingFolder].first].second = pathToBlobsInCurStaggingArea[posStaggingArea].second;
            posWorkingFolder++;
            posStaggingArea++;
        }
        else {
            this->status[pathToBlobsInCurWorkingFolder[posWorkingFolder].first].first = "unchanged";
            posWorkingFolder++;
            posStaggingArea++;
        }

    }

    std::sort(pathToTreesInCurWorkingFolder.begin(), pathToTreesInCurWorkingFolder.end());
    std::sort(pathToTreesInCurStaggingArea.begin(), pathToTreesInCurStaggingArea.end());
    posWorkingFolder = 0;
    posStaggingArea = 0;
    while (posWorkingFolder < (int)pathToTreesInCurWorkingFolder.size() || posStaggingArea < (int)pathToTreesInCurStaggingArea.size()) {

        if (posWorkingFolder == (int)pathToTreesInCurWorkingFolder.size()) {
            //if(verbose){std::cout << "Dossier supprimé : " << relativeToRepo(pathToTreesInCurStaggingArea[posStaggingArea].first) << std::endl;}
            this->deleted.push_back(pathToTreesInCurStaggingArea[posStaggingArea].first);
            this->status[pathToTreesInCurStaggingArea[posStaggingArea].first].first = "deleted";
            this->status[pathToTreesInCurStaggingArea[posStaggingArea].first].second = pathToTreesInCurStaggingArea[posStaggingArea].second;
            posStaggingArea++;
        }
        else if (posStaggingArea == (int)pathToTreesInCurStaggingArea.size()) {
            //if(verbose){std::cout << "Dossier ajouté : " << relativeToRepo(pathToTreesInCurWorkingFolder[posWorkingFolder].first) << std::endl;}
            this->added.push_back(pathToTreesInCurWorkingFolder[posWorkingFolder].first);
            this->status[pathToTreesInCurWorkingFolder[posWorkingFolder].first].first = "added";
            posWorkingFolder++;
        }
        else if (pathToTreesInCurWorkingFolder[posWorkingFolder].first > pathToTreesInCurStaggingArea[posStaggingArea].first) {
            //if(verbose){std::cout << "Dossier supprimé : " << relativeToRepo(pathToTreesInCurStaggingArea[posStaggingArea].first) << std::endl;}
            this->deleted.push_back(pathToTreesInCurStaggingArea[posStaggingArea].first);
            this->status[pathToTreesInCurStaggingArea[posStaggingArea].first].first = "deleted";
            this->status[pathToTreesInCurStaggingArea[posStaggingArea].first].second = pathToTreesInCurStaggingArea[posStaggingArea].second;
            posStaggingArea++;
        }
        else if (pathToTreesInCurStaggingArea[posStaggingArea].first > pathToTreesInCurWorkingFolder[posWorkingFolder].first) {
            //if(verbose){std::cout << "Dossier ajouté : " << relativeToRepo(pathToTreesInCurWorkingFolder[posWorkingFolder].first) << std::endl;}
            this->added.push_back(pathToTreesInCurWorkingFolder[posWorkingFolder].first);
            this->status[pathToTreesInCurWorkingFolder[posWorkingFolder].first].first = "added";
            posWorkingFolder++;
        }
        else if (pathToTreesInCurStaggingArea[posStaggingArea].second != pathToTreesInCurWorkingFolder[posWorkingFolder].second) {
            //if(verbose){std::cout << "Dossier modifié : " << relativeToRepo(pathToTreesInCurWorkingFolder[posWorkingFolder].first) << std::endl;}
            this->modified.push_back(pathToTreesInCurWorkingFolder[posWorkingFolder].first);
            calc_differences(pathToTreesInCurWorkingFolder[posWorkingFolder].first, pathToTreesInCurStaggingArea[posStaggingArea].second, verbose);
            this->status[pathToTreesInCurWorkingFolder[posWorkingFolder].first].first = "modified";
            this->status[pathToTreesInCurWorkingFolder[posWorkingFolder].first].second = pathToTreesInCurStaggingArea[posStaggingArea].second;
            posWorkingFolder++;
            posStaggingArea++;
        }
        else {
            calc_differences(pathToTreesInCurWorkingFolder[posWorkingFolder].first, pathToTreesInCurStaggingArea[posStaggingArea].second, verbose);
            this->status[pathToTreesInCurWorkingFolder[posWorkingFolder].first].first = "unchanged";
            posWorkingFolder++;
            posStaggingArea++;
        }

    }


}

StaggingArea::StaggingArea(PtitGitRepos repos) {

    this->repos = repos;
    this->rootTree = repos.get_repos_content("index/INDEX");

    this->construct_tree(this->rootTree);
    this->calc_differences();
}

std::string StaggingArea::get_root_tree() {

    return this->rootTree;

}

void StaggingArea::add(fs::path pathToAdd) {


    if (this->status[pathToAdd].first == "unchanged") {
        std::cout << "Last version of this object already added" << std::endl;
        return;
    }
    if (this->status[pathToAdd].first == "") {
        std::cout << "Invalid object" << std::endl;
        return;
    }

    if (fs::is_regular_file(pathToAdd)) {

        if (this->status[pathToAdd].first == "deleted") {

            std::string hashedContent = this->status[pathToAdd].second;
            std::string fatherHash = this->treeStaggingAreaReversed[hashedContent];
            std::string updatedFather = delete_object(this->repos.get_repos_content(fs::path("index") / get_path_to_object(fatherHash)), hashedContent).first;
            std::string hashUpdatedFather = hashString(updatedFather);
            this->write_content(updatedFather, hashUpdatedFather);
            std::string grandFatherHash = this->treeStaggingAreaReversed[fatherHash];
            update_node(grandFatherHash, fatherHash, hashUpdatedFather);

        }

        else if (this->status[pathToAdd].first == "modified") {

            std::string pastHashedContent = this->status[pathToAdd].second;
            std::string curContent = this->repos.get_working_folder_content(pathToAdd);
            curContent = "file " + std::to_string(curContent.size() + std::string(relativeToRepo(pathToAdd)).size() + 1) + '\n' + std::string(relativeToRepo(pathToAdd)) + '\n' + curContent;
            std::string curHashedContent = hashString(curContent);
            this->write_content(curContent, curHashedContent);
            std::string pastFatherHash = this->treeStaggingAreaReversed[pastHashedContent];
            update_node(pastFatherHash, pastHashedContent, curHashedContent);

        }

        else if (this->status[pathToAdd].first == "added") {

            std::string curContent = this->repos.get_working_folder_content(pathToAdd);
            curContent = "file " + std::to_string(curContent.size() + std::string(relativeToRepo(pathToAdd)).size() + 1) + '\n' + std::string(relativeToRepo(pathToAdd)) + '\n' + curContent;
            std::string curHashedContent = hashString(curContent);
            this->write_content(curContent, curHashedContent);
            fs::path fatherPath = pathToAdd.parent_path();
            std::string pastFatherHash = this->status[fatherPath].second;
            std::string pastFatherContent = this->repos.get_repos_content(fs::path("index") / get_path_to_object(pastFatherHash));
            std::string newFatherContent = insert_new_object(pastFatherContent, "file", curHashedContent, pathToAdd.filename());
            std::string newFatherHash = hashString(newFatherContent);
            this->write_content(newFatherContent, newFatherHash);
            std::string pastGrandFatherHash = this->treeStaggingAreaReversed[pastFatherHash];
            update_node(pastGrandFatherHash, pastFatherHash, newFatherHash);

        }
    }

    else {

        if (this->status[pathToAdd].first == "deleted") {

            std::string hashedContent = this->status[pathToAdd].second;
            std::string fatherHash = this->treeStaggingAreaReversed[hashedContent];
            std::string updatedFather = delete_object(this->repos.get_repos_content(fs::path("index") / get_path_to_object(fatherHash)), hashedContent).first;
            std::string hashUpdatedFather = hashString(updatedFather);
            this->write_content(updatedFather, hashUpdatedFather);
            std::string grandFatherHash = this->treeStaggingAreaReversed[fatherHash];
            update_node(grandFatherHash, fatherHash, hashUpdatedFather);

        }

        else if (this->status[pathToAdd].first == "modified") {

            Tree curTree = Tree(pathToAdd);
            this->add_all(curTree);
            std::string pastHashedContent = this->status[pathToAdd].second;
            std::string pastFatherHash = this->treeStaggingAreaReversed[pastHashedContent];
            update_node(pastFatherHash, pastHashedContent, curTree.getHashedContent());

        }

        else if (this->status[pathToAdd].first == "added") {

            Tree curTree = Tree(pathToAdd);
            this->add_all(curTree);
            std::string curHashedContent = curTree.getHashedContent();
            fs::path fatherPath = pathToAdd.parent_path();
            std::string pastFatherHash = this->status[fatherPath].second;
            std::string pastFatherContent = this->repos.get_repos_content(fs::path("index") / get_path_to_object(pastFatherHash));
            std::string newFatherContent = insert_new_object(pastFatherContent, "tree", curHashedContent, pathToAdd.filename());
            std::string newFatherHash = hashString(newFatherContent);
            this->write_content(newFatherContent, newFatherHash);
            this->update_node(this->treeStaggingAreaReversed[pastFatherHash], pastFatherHash, newFatherHash);

        }

    }

}



void StaggingArea::add_all() {

    Tree rootTree = Tree(this->repos.getWorkingFolder());

    std::ofstream INDEX(this->repos.getWorkingFolder() / ".ptitgit/index/INDEX");
    INDEX << rootTree.getHashedContent();
    INDEX.close();

    add_all(Tree(this->repos.getWorkingFolder()));

}

void StaggingArea::add_all(Tree curTree) {

    this->write_content(curTree);

    std::vector<File> filesInside = curTree.get_blobs_inside();
    for (size_t i = 0; i < filesInside.size(); i++) {
        this->write_content(filesInside[i]);
    }

    std::vector<Tree> treesInside = curTree.get_trees_inside();
    for(size_t i = 0; i < treesInside.size(); i++) {
        this->add_all(treesInside[i]);
    }

}

void StaggingArea::write_content(Object curObject) {

    write_content(curObject.getContent(), curObject.getHashedContent());

}

void StaggingArea::write_content(std::string content, std::string hashedContent) {

    if (!fs::exists(get_folder_to_object(hashedContent))) {
        fs::create_directory(this->repos.getWorkingFolder() / ".ptitgit/index" / get_folder_to_object(hashedContent));
    }

    std::ofstream fileToComplete(this->repos.getWorkingFolder() / ".ptitgit/index" / get_path_to_object(hashedContent));
    fileToComplete << content;
    fileToComplete.close();

}

void StaggingArea::update_node(std::string curHash, std::string hashToDelete, std::string hashToInsert) {

    if (this->treeStaggingArea[curHash].empty()) {
        std::ofstream INDEX(this->repos.getWorkingFolder() / ".ptitgit/index/INDEX");
        INDEX << hashToInsert;
        INDEX.close();
        return;
    }

    std::string content = this->repos.get_repos_content(fs::path("index") / get_path_to_object(curHash));

    std::pair<std::string, std::string> infoDelete = delete_object(content, hashToDelete);
    std::string updatedContent1 = infoDelete.first;
    std::string deletedLine = infoDelete.second;

    std::string updatedContent2 = insert_new_object(updatedContent1, get_object_type(deletedLine), hashToInsert, get_object_path(deletedLine));

    this->write_content(updatedContent2, hashString(updatedContent2));

    if (this->treeStaggingAreaReversed[curHash] == "") { //Means that we are on the root tree, so we must add it to INDEX
        std::ofstream INDEX(this->repos.getWorkingFolder() / ".ptitgit/index/INDEX");
        INDEX << hashString(updatedContent2);
        INDEX.close();
    }
    else {
        update_node(this->treeStaggingAreaReversed[curHash], curHash, hashString(updatedContent2));
    }

}

void StaggingArea::get_stagging_area() {
    
    std::cout << "Added : " << std::endl;
    for (std::string a : added) {
        std::cout << "   " << a << std::endl;
    }
    std::cout << std::endl;

    std::cout << "Deleted : " << std::endl;
    for (std::string d : deleted) {
        std::cout << "   " << d << std::endl;
    }
    std::cout << std::endl;

    std::cout << "Modified : " << std::endl;
    for (std::string m : modified) {
        std::cout << "   " << m << std::endl;
    }
    std::cout << std::endl;

}


