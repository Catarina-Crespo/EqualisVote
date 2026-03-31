//
// Created by Catarina on 15/07/2025.
//
#include "protocol_run_colbac.hpp"
#include <iostream>
#include <vector>
#include <algorithm>
#include <cassert>
#include <cstring>
#include "parties.hpp"
#include "random_permutation.hpp"
#include "verifiable_exponentiation.hpp"
#include "fiat_shamir.hpp"
#include "repeated_sigma_protocol.hpp"
#include "shuffle_compatible_dleq.hpp"
#include "shuffled_sigma_protocol.hpp"
#include "utilities.hpp"
#include "base_point.hpp"

void protocol_runc(size_t initial_group_size, size_t n_voting_users) {

    assert(initial_group_size >= n_voting_users);
    
    Server server;
    Server thirdParty;
    Group group;
    std::vector<User> users;

    // Create users
    std::cout << "U.Register..." << std::endl;

    // prepare DLOG proof engine
    DLOG2GenProof dlog2gen_pf;
    VEP<BatchedDLEQProof> ve(NULL);

    for (auto i = 0; i < initial_group_size; i++)
    {
        User user;
        UserRegister(user, server);
        assert(dlog2gen_pf.verify(user.upk_u_i, user.upk_proof));
        users.push_back(user);
    }

    // Creates group on server and adds users
    std::cout << "S.CreateGroup..." << std::endl;
    server.create_group(group);     
    assert(dlog2gen_pf.verify(server.spk_G, server.spk_proof));

    std::cout << "U.JoinGroup..." << std::endl;
    for (auto i = 0; i < initial_group_size; i++)
    {
        users[i].join_group(group);     
    }

    // One user creates an election
    Election election;
    server.create_election(election, 0, 2);
    std::vector<int> sent_votes;

    // A part of the users vote on the election
    std::cout << "U.ElectionVote..." << std::endl;
    for (auto i = 0; i < n_voting_users; i++)
    {
        uint32_t rand_int = randombytes_random() % (election.max_value - election.min_value + 1);
        rand_int += election.min_value;
        int vote = (int)rand_int;
        sent_votes.push_back(vote);
        UserElectionVote(users[i], election, vote, server);
    }

    // User does Init and exponentiates/shuffles the tags and sends it to the Server
    // RO salt
    unsigned char salt[crypto_hash_sha256_BYTES];
    randombytes_buf(salt, crypto_hash_sha256_BYTES);

    // generate alpha obfuscator scalar
    unsigned char alpha[crypto_core_ristretto255_SCALARBYTES];
    assert(2 * crypto_core_ristretto255_SCALARBYTES >= crypto_hash_sha256_BYTES);
    unsigned char alpha_hash[2 * crypto_core_ristretto255_SCALARBYTES]; // read note on crypto_core_ristretto255_scalar_reduce
    memset(alpha_hash, 0, 2 * crypto_core_ristretto255_SCALARBYTES);
    crypto_hash_sha256_state state;
    crypto_hash_sha256_init(&state);
    crypto_hash_sha256_update(&state, salt, crypto_hash_sha256_BYTES);
    for (auto const& x : group.user_tokens)
    {
        std::string key = x.first;
        unsigned char *val = x.second;
        crypto_hash_sha256_update(&state, val, crypto_core_ristretto255_BYTES);
    }
    crypto_hash_sha256_final(&state, alpha_hash);
    crypto_core_ristretto255_scalar_reduce(alpha, alpha_hash);

    // generate obfuscated tag list T1, ..., Tn
    unsigned char *T[initial_group_size];
    for (auto i = 0; i < initial_group_size; i++)
    {
        T[i] = (unsigned char *)malloc(crypto_core_ristretto255_BYTES);
    }
    size_t ctr = 0;
    for (auto const& x : group.user_tokens)
    {
        std::string key = x.first;
        unsigned char *val = x.second;
        if (crypto_scalarmult_ristretto255(T[ctr], alpha, val) != 0)
        {
            printf("failed to compute T[%zu]\n", ctr);
            abort();
        }
        ctr++;
    }

    // Server shuffles and exponentiates both the received tags and the set of encrypted ballots
    size_t rho_i_star_G[initial_group_size];
    random_permutation(initial_group_size, rho_i_star_G);

    unsigned char *Tprime[initial_group_size];
    for (auto i = 0; i < initial_group_size; i++) {
        Tprime[i] = (unsigned char *) malloc(crypto_core_ristretto255_BYTES);
    }

    // perform VPE evaluation - .TODO: I don't understand this part - this only creates instances of these things
    ShuffledSigmaProtocol<SCDLEQSigmaProtocol> pfsys(initial_group_size);
    RepeatedSigmaProtocol<ShuffledSigmaProtocol<SCDLEQSigmaProtocol>> repfsys(128, pfsys);
    FiatShamir<RepeatedSigmaProtocol<ShuffledSigmaProtocol<SCDLEQSigmaProtocol>>> fsproof(repfsys);
    VEP<FiatShamir<RepeatedSigmaProtocol<ShuffledSigmaProtocol<SCDLEQSigmaProtocol>>>> vep(&fsproof);

    std::cout << "Printing election s_E and epk_E\n";
    printhex(election.s_E, crypto_core_ristretto255_SCALARBYTES);
    printhex(election.epk_E, crypto_core_ristretto255_BYTES);

    // --------------- DEBUG
    std::vector<bool> seen(initial_group_size, false);
    for (size_t i = 0; i < initial_group_size; ++i) {
        assert(rho_i_star_G[i] < initial_group_size);
        assert(!seen[rho_i_star_G[i]]);
        seen[rho_i_star_G[i]] = true;
    }

    for (size_t i = 0; i < initial_group_size; ++i) {
        if (!crypto_core_ristretto255_is_valid_point(T[i])) {
            std::cerr << "T[" << i << "] is not a valid Ristretto255 point!" << std::endl;
            assert(false);
        }
    }

    assert(crypto_core_ristretto255_is_valid_point(election.epk_E));



    unsigned char S_shuffle_exp_proof[vep.exponentiation_proof_BYTES];
    if (vep.eval(
        election.s_E, election.epk_E,
        initial_group_size, T,
        rho_i_star_G,
        Tprime,
        S_shuffle_exp_proof
    ) != 0) {
        assert(false);
    }

    // Now W_s
    auto upk_i_star_str = hex_to_string(election.epk_E, crypto_core_ristretto255_BYTES);
    auto entry = server.ballots.find(upk_i_star_str);       // c. Recover the entry from the server related with i_star (external user)
    bool election_exists = (entry != server.ballots.end());
    assert(election_exists);
    BallotVector vec_of_ballots = entry->second;            // c. Recover the vector of ballots
    size_t n_ballots = vec_of_ballots.size();
    std::cout << "Ballots present: " << n_ballots << std::endl;

    // extract ballots and prepare for server's VE
    unsigned char *W[n_ballots];
    unsigned char *W_s[n_ballots];
    // c. Allocate memory for the ballots
    for (auto i = 0; i < n_ballots; i++) {
        W[i] = (unsigned char *) malloc(crypto_core_ristretto255_BYTES);
        W_s[i] = (unsigned char *) malloc(crypto_core_ristretto255_BYTES);
    }
    ctr = 0;
    // c. Copy the ballots stored by the external user into W
    for(const auto& ballot : vec_of_ballots)
    {
        memcpy(W[ctr], ballot, crypto_core_ristretto255_BYTES);
        ctr++;
    }

    // perform Server's VE on ballots
    unsigned char S_send_votes_proof[ve.exponentiation_proof_BYTES];
    if (ve.eval(
        server.s_G, server.spk_G,
        n_ballots, W,
        NULL,
        W_s,
        S_send_votes_proof
    ) != 0) {
        assert(false);
    }

    assert(ve.check(
        server.spk_G,
        n_ballots,
        W,
        W_s,
        S_send_votes_proof
    ));

    // Server sends both things to the User

    // User exponentiates it to the inverse of alpha and performs the intersection
    std::cout << "G.IntersectVotes..." << std::endl;
    unsigned char inv_alpha[crypto_core_ristretto255_SCALARBYTES];
    crypto_core_ristretto255_scalar_invert(inv_alpha, alpha);
    unsigned char *Tpp[initial_group_size];

    // c. Exponentiates Tpp to the inverse of alpha
    for (auto i = 0; i < initial_group_size; i++)
    {
        Tpp[i] = (unsigned char *)malloc(crypto_core_ristretto255_BYTES);
        if (crypto_scalarmult_ristretto255(Tpp[i], inv_alpha, Tprime[i]) != 0)
        {
            printf("could not exp to the 1/alpha exponentiation\n");
            abort();
        }
    }
    std::vector<int> X; // recovered votes
    unsigned char yw[crypto_core_ristretto255_BYTES];
    unsigned char recovered_yw[crypto_core_ristretto255_BYTES];
    unsigned char encoded_candidate_vote[crypto_core_ristretto255_SCALARBYTES];
    memset(encoded_candidate_vote, 0x00, crypto_core_ristretto255_SCALARBYTES);

    // c. Takes each tag
    for (auto y_indx = 0; y_indx < initial_group_size; y_indx++)
    {
        // c. Takes each encrypted ballot
        for (auto w_indx = 0; w_indx < n_ballots; w_indx++)
        {
            // c. For each possible value for a vote
            for (int candidate_vote = election.min_value; candidate_vote <= election.max_value; candidate_vote++)
            {
                encoded_candidate_vote[0] = (unsigned char)candidate_vote;
                crypto_core_ristretto255_add(yw, Tpp[y_indx], W_s[w_indx]); // yw = T3p * W_s
                // due to a quirk in crypto_scalarmult_ristretto255, the 0-vote is handled separatedly
                if (candidate_vote == 0) {
                    ristretto255_identity_point(recovered_yw);
                } else {
                    // c. Compute b = a^k, crypto_scalarmult_ristretto255(b, k, a) != 0
                    if (crypto_scalarmult_ristretto255(recovered_yw, encoded_candidate_vote, server.spk_G) != 0)
                    {
                        printf("failed to exp to the x in intersect votes\n");
                        printhex(encoded_candidate_vote, crypto_core_ristretto255_SCALARBYTES);
                        abort();
                    }
                }
                // if yw matches recovered_yw, we identified a vote
                if (memcmp(yw, recovered_yw, crypto_core_ristretto255_BYTES) == 0) {
                    X.push_back(candidate_vote);
                }
            }
        }
    }

    // check for exact match in votes
    std::sort(X.begin(), X.end());
    std::sort(sent_votes.begin(), sent_votes.end());
    std::cout << "Printing sent votes: \n";
    assert(sent_votes == X);
    for (int i = 0; i < n_voting_users; i++) {
        std::cout << sent_votes[i];
    }
    std::cout << std::endl;

    std::cout << "Printing X votes: \n";
    for (int i = 0; i < n_voting_users; i++) {
        std::cout << X[i];
    }
    std::cout << std::endl;

    for (auto i = 0; i < initial_group_size; i++)
    {
        free(T[i]);
        free(Tprime[i]);
        free(Tpp[i]);
    }
    for (auto i = 0; i < n_voting_users; i++)
    {
        free(W[i]);
        free(W_s[i]);
    }

}

int main() {
    protocol_runc(40, 30);

    std::cout << "The simulation completed successfully\n";
    return 0;
}