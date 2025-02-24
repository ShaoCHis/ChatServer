#ifndef GROUPUSER_H
#define GROUPUSER_H

#include "user.hpp"

// 群组用户，多了一个role角色信息，从User类直接继承，复用User的其他信息
class GroupUser : public User
{
public:
    GroupUser() = default;

    GroupUser(int id, std::string name, std::string state, std::string role) : User(id,name,"",state) { this->role = role; };

    void setRole(std::string role) { this->role = role; }

    std::string getRole() { return this->role; }

private:
    std::string role;
};

#endif