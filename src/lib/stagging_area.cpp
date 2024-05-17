#include "repos.h"
#include "stagging_area.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;

std::string get_next_line(std::string &s) {

    size_t pos = s.find('\n');

    if(pos == std::string::npos) {
        throw std::runtime_error("No newline found is the string");
    }
    
    std::string nextLine = s.substr(0, pos);
    s = s.substr(pos + 1, s.size());
    return nextLine;

}

std::map<std::string, std::string> cut_line(std::string contentLine) {

    std::map<std::string, std::string> m;

    size_t pos1 = contentLine.find(' ');
    m["type"] = contentLine.substr(0, pos1);

    contentLine = contentLine.substr(pos1 + 1, contentLine.size());

    size_t pos2 = contentLine.find(' ');
    m["hash"] = contentLine.substr(0, pos2);

    contentLine = contentLine.substr(pos2 + 1, contentLine.size());

    size_t pos3 = contentLine.find(' ');
    m["path"] = contentLine.substr(0, pos3);

    return m;

}

std::string get_object_type(std::string contentLine) {

    return cut_line(contentLine)["type"];

}

std::string get_object_hash(std::string contentLine) {

    return cut_line(contentLine)["hash"];

}

std::string get_object_path(std::string contentLine) {

    return cut_line(contentLine)["path"];

}

void StaggingArea::construct_tree(std::string curFileHash) {

    // TODO : fix this
    /*
    std::string fileContent = this->repos.get_repos_content(fs::path("index/objects") / get_path_to_object(curFileHash));

    std::pair<std::string, std::string> separatedContent = get_next_line(fileContent);

    if (separatedContent.first == "tree") {
        try {
            fileContent = get_next_line(separatedContent.second).second; //Erase file name
            while(fileContent != "") {
                separatedContent = get_next_line(fileContent);
                std::string nextFileHash = get_file_hash(separatedContent.first);
                this->treeStaggingArea[curFileHash].push_back(nextFileHash);
                fileContent = separatedContent.second;
            }
        }
        catch (const std::runtime_error &e) {
            
        }
    }
    else if (separatedContent.second == "blob") {
        
    }
    else {
        std::cerr << "Error in StaggingArea::construct_tree : invalid file type (" << separatedContent.first << ")\n";
    }
    */

}

void StaggingArea::show_differences() {

    show_differences(repos.getWorkingFolder(), repos.get_repos_content("index/INDEX"));

}

void StaggingArea::show_differences(fs::path curPathInWorkingArea, std::string curHashStaggingArea) {

    Tree curTreeInWorkingArea = Tree(curPathInWorkingArea);

    std::string curContentStaggingArea = repos.get_repos_content(fs::path("index") / get_path_to_object(curHashStaggingArea));
    get_next_line(curContentStaggingArea);

    fs::path curPathStaggingArea = fs::path(get_next_line(curContentStaggingArea));

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

    std::sort(pathToBlobsInCurWorkingFolder.begin(), pathToBlobsInCurWorkingFolder.end());
    std::sort(pathToBlobsInCurStaggingArea.begin(), pathToBlobsInCurStaggingArea.end());
    int posWorkingFolder = 0, posStaggingArea = 0;
    while (posWorkingFolder < (int)pathToBlobsInCurWorkingFolder.size() || posStaggingArea < (int)pathToBlobsInCurStaggingArea.size()) {

        if (posWorkingFolder == (int)pathToBlobsInCurWorkingFolder.size()) {
            std::cout << "Fichier supprimé : " << pathToBlobsInCurStaggingArea[posStaggingArea].first << std::endl;
            posStaggingArea++;
        }
        else if (posStaggingArea == (int)pathToBlobsInCurStaggingArea.size()) {
            std::cout << "Fichier ajouté : " << pathToBlobsInCurWorkingFolder[posWorkingFolder].first << std::endl;
            posWorkingFolder++;
        }
        else if (pathToBlobsInCurWorkingFolder[posWorkingFolder].first > pathToBlobsInCurStaggingArea[posStaggingArea].first) {
            std::cout << "Fichier supprimé : " << pathToBlobsInCurWorkingFolder[posWorkingFolder].first << std::endl;
            posStaggingArea++;
        }
        else if (pathToBlobsInCurStaggingArea[posStaggingArea].first > pathToBlobsInCurWorkingFolder[posWorkingFolder].first) {
            std::cout << "Fichier ajouté : " << pathToBlobsInCurWorkingFolder[posWorkingFolder].first << std::endl;
            posWorkingFolder++;
        }   
        else if (pathToBlobsInCurStaggingArea[posStaggingArea].second != pathToBlobsInCurWorkingFolder[posWorkingFolder].second) {
            std::cout << "Fichier modifié : " << pathToBlobsInCurWorkingFolder[posWorkingFolder].first << std::endl;
            posWorkingFolder++;
            posStaggingArea++;
        }
        else {
            posWorkingFolder++;
            posStaggingArea++;
        }

    }

    std::sort(pathToTreesInCurWorkingFolder.begin(), pathToTreesInCurWorkingFolder.end());
    std::sort(pathToTreesInCurStaggingArea.begin(), pathToTreesInCurStaggingArea.end());
    posWorkingFolder = 0;
    posStaggingArea = 0;
    while (posWorkingFolder < (int)pathToTreesInCurWorkingFolder.size() || posStaggingArea < (int)pathToTreesInCurStaggingArea.size()) {

        if (posWorkingFolder == (int)pathToTreesInCurWorkingFolder.size() || pathToTreesInCurWorkingFolder[posWorkingFolder].first > pathToTreesInCurStaggingArea[posStaggingArea].first) {
            std::cout << "Dossier supprimé : " << pathToTreesInCurStaggingArea[posStaggingArea].first << std::endl;
            posStaggingArea++;
        }
        else if (posStaggingArea == (int)pathToTreesInCurStaggingArea.size()) {
            std::cout << "Dossier ajouté : " << pathToTreesInCurWorkingFolder[posWorkingFolder].first << std::endl;
            posWorkingFolder++;
        }
        else if (pathToTreesInCurWorkingFolder[posWorkingFolder].first > pathToTreesInCurStaggingArea[posStaggingArea].first) {
            std::cout << "Dossier supprimé : " << pathToTreesInCurStaggingArea[posStaggingArea].first << std::endl;
            posStaggingArea++;
        }
        else if (pathToTreesInCurStaggingArea[posStaggingArea].first > pathToTreesInCurWorkingFolder[posWorkingFolder].first) {
            std::cout << "Dossier ajouté : " << pathToTreesInCurWorkingFolder[posWorkingFolder].first << std::endl;
            posWorkingFolder++;
        }
        else if (pathToTreesInCurStaggingArea[posStaggingArea].second != pathToTreesInCurWorkingFolder[posWorkingFolder].second) {
            std::cout << "Dossier modifié : " << pathToTreesInCurWorkingFolder[posWorkingFolder].first << std::endl;
            show_differences(pathToTreesInCurWorkingFolder[posWorkingFolder].first, pathToTreesInCurStaggingArea[posStaggingArea].second);
            posWorkingFolder++;
            posStaggingArea++;
        }
        else {
            show_differences(pathToTreesInCurWorkingFolder[posWorkingFolder].first, pathToTreesInCurStaggingArea[posStaggingArea].second);
            posWorkingFolder++;
            posStaggingArea++;
        }

    }

}

StaggingArea::StaggingArea(PtitGitRepos repos) {

    this->repos = repos;
    this->rootTree = repos.get_repos_content("index/INDEX");

    this->construct_tree(this->rootTree);
}

std::string StaggingArea::get_root_tree() {

    return this->rootTree;

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

    if (!fs::exists(get_folder_to_object(curObject.getHashedContent()))) {
        fs::create_directory(this->repos.getWorkingFolder() / ".ptitgit/index" / get_folder_to_object(curObject.getHashedContent()));
    }

    std::ofstream fileToComplete(this->repos.getWorkingFolder() / ".ptitgit/index" / get_path_to_object(curObject.getHashedContent()));
    fileToComplete << curObject.getContent();
    fileToComplete.close();

}

void StaggingArea::add_file(File fileToAdd) {

}

void StaggingArea::add_tree(Tree treeToAdd) {
    
}
