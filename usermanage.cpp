#include "usermanage.h"
#include "utils.h"
#include <iostream>
#include <fstream>
#include "wrong.h"
#include <QCryptographicHash>
#include <QString>

UserManage::UserManage() {}

// 密码加密函数
std::string UserManage::encryptPassword(const std::string& password) {
    QByteArray hash = QCryptographicHash::hash(
        QByteArray::fromStdString(password),
        QCryptographicHash::Sha256
        );
    return hash.toHex().toStdString();
}

void UserManage::createUser(std::string username, std::string password) {
    // 加密密码
    std::string encryptedPwd = encryptPassword(password);

    std::ofstream file("../../res/user.txt", std::ios::app);
    if(!file.is_open()) {
        std::cerr << "Failed to open file";
        return;
    }
    file << username << "\t" << encryptedPwd << "\n";
    file.close();
}

void UserManage::dropUser(std::string username) {
    std::string filename = "../../res/user.txt";
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return;
    }

    std::vector<std::string> lines;
    std::string line;

    while (std::getline(file, line)) {
        std::vector<std::string> userInfo = utils::split(line, "\t");
        if (userInfo.empty()) continue;

        if (userInfo[0] != username) {
            lines.push_back(line);
        }
    }
    file.close();

    std::ofstream outFile(filename);
    if (!outFile.is_open()) {
        std::cerr << "Failed to open file for writing: " << filename << std::endl;
        return;
    }

    for (const auto& l : lines) {
        outFile << l << std::endl;
    }
    outFile.close();
}

bool UserManage::findUser(std::string username, std::string password) {
    std::string filename = "../../res/user.txt";
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return false;
    }

    std::string line;
    bool userExists = false;
    std::string encryptedInput = encryptPassword(password);

    while (std::getline(file, line)) {
        std::vector<std::string> userInfo = utils::split(line, "\t");
        if (userInfo.size() < 2) continue;

        if (userInfo[0] == username) {
            userExists = true;
            if (userInfo[1] == encryptedInput) {
                file.close();
                return true;
            } else {
                Wrong::getInstance("The password input is wrong for this user.")->show();
                //Wrong* wrong = new Wrong("The password input is wrong for this user.");
                //wrong->show();
                file.close();
                return false;
            }
        }
    }
    file.close();

    if (!userExists) {
        Wrong::getInstance("This user does not exist.")->show();
    }
    return false;
    //return true;
}
