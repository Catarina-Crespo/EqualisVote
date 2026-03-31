#pragma once

#ifdef __cplusplus
extern "C"{
#endif
#include <sodium.h>
#ifdef __cplusplus
}
#endif

#include <cstdlib>
#include <string>
#include <map>
#include <iostream>
#include <vector>
#include "dlog_to_gen.hpp"


typedef std::map<std::string, unsigned char *> UserTokenMap;

class Election {
public:
    unsigned char s_E[crypto_core_ristretto255_SCALARBYTES];
    unsigned char epk_E[crypto_core_ristretto255_BYTES];
    unsigned char epk_proof[DLOG2GenProof::proof_BYTES];
    int min_value;
    int max_value;
};

class Group {
public:
    unsigned char spk_G[crypto_core_ristretto255_BYTES];
    UserTokenMap user_tokens;
    ~Group()
    {
        // clear the hashmap
        for (auto const& x : user_tokens)
        {
            std::string key = x.first;
            unsigned char *val = x.second;
            std::cout << "deleting token for " << x.first << std::endl;
            free(val);
        }
    }
};


class User {
public:
    static const size_t upk_BYTES = crypto_core_ristretto255_BYTES;
    static const size_t u_BYTES = crypto_core_ristretto255_SCALARBYTES;
    static const size_t v_BYTES = crypto_core_ristretto255_SCALARBYTES;
    unsigned char v_i[crypto_core_ristretto255_SCALARBYTES];
    unsigned char u_i[crypto_core_ristretto255_SCALARBYTES];
    unsigned char upk_u_i[crypto_core_ristretto255_BYTES];
    unsigned char upk_proof[DLOG2GenProof::proof_BYTES];
    void join_group(Group &group);
};

typedef std::vector<unsigned char *> BallotVector;
typedef std::map<std::string, BallotVector> BallotsMap; // c. u_i_str, ballots associated

class Server {
    public:
    BallotsMap ballots;
    unsigned char s_G[crypto_core_ristretto255_SCALARBYTES];
    unsigned char spk_G[crypto_core_ristretto255_BYTES];
    unsigned char spk_proof[DLOG2GenProof::proof_BYTES];
    unsigned char s_E[crypto_core_ristretto255_SCALARBYTES];
    unsigned char epk_E[crypto_core_ristretto255_BYTES];
    unsigned char epk_proof[DLOG2GenProof::proof_BYTES];
    void register_user(User &user);
    void add_ballot(User &votee, unsigned char *ballot);
    void add_ballot(Election &election, unsigned char *ballot);
    void create_group(Group &group);
    void create_election(Election &election, int min_value, int max_value);
    ~Server()
    {
        // clear the hashmap
        for (auto const& x : ballots)
        {
            std::string key = x.first;
            BallotVector val = x.second;
            std::cout << "deleting ballots for " << x.first << std::endl;
            for(const auto& ballot : val) 
            {
                free(ballot);
            }
        }
    }
};

void UserRegister(User &user, Server &server);
void UserVote(User &voter, User &votee, int vote, Server &server);
void UserElectionVote(User &voter, Election &election, int vote, Server &server);