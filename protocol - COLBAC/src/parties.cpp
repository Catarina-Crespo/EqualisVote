#include "parties.hpp"
#include "utilities.hpp"
#include <utility>
#include <cstring>
#include "verifiable_exponentiation.hpp"


// Function for User_i to join the group (U.JoinGroup)
void User::join_group(Group &group)
{
    DLOGProof dlog_pf;

    // Compute -v_i
    unsigned char neg_v_i[crypto_core_ristretto255_SCALARBYTES];
    crypto_core_ristretto255_scalar_negate(neg_v_i, v_i);

    // Compute z_i_G = (spk_G)^(-v_i)
    unsigned char *z_i_G = (unsigned char *)malloc(crypto_core_ristretto255_BYTES); // freed during Group destruction
    if (crypto_scalarmult_ristretto255(z_i_G, neg_v_i, group.spk_G) != 0)
    {
        printf("failed computing z_i_G\n");
        abort();
    }
    // compute a proof for z_i_G
    unsigned char user_token_statement[DLOGProof::statement_BYTES];
    unsigned char user_token_proof[DLOGProof::proof_BYTES];
    memcpy(user_token_statement, group.spk_G, crypto_core_ristretto255_BYTES);
    memcpy(user_token_statement + crypto_core_ristretto255_BYTES, z_i_G, crypto_core_ristretto255_BYTES);
    dlog_pf.prove(neg_v_i, user_token_statement, user_token_proof);

    // send z_i_G to the group
    assert(dlog_pf.verify(user_token_statement, user_token_proof));
    auto upk_str = hex_to_string(upk_u_i, upk_BYTES);
    bool user_exists = (group.user_tokens.find(upk_str) != group.user_tokens.end());
    assert(!user_exists);
    group.user_tokens.insert(UserTokenMap::value_type(upk_str, z_i_G));

    // user could maintain a local copy of group.user_tokens if desired

    std::cout << "User " << upk_str
        << " joined group with token "
        << hex_to_string(z_i_G, crypto_core_ristretto255_BYTES)
        << std::endl;
}

void Server::register_user(User &user)
{
    auto upk_str = hex_to_string(user.upk_u_i, user.upk_BYTES);
    bool user_exists = (ballots.find(upk_str) != ballots.end());
    assert(!user_exists);
    ballots.insert(BallotsMap::value_type(upk_str, BallotVector()));
}

void Server::add_ballot(User &votee, unsigned char *ballot)
{
    auto upk_str = hex_to_string(votee.upk_u_i, votee.upk_BYTES);
    auto entry = ballots.find(upk_str);
    bool user_exists = (entry != ballots.end());
    assert(user_exists);
    entry->second.push_back(ballot);
}

void Server::add_ballot(Election &election, unsigned char *ballot)
{
    auto upk_str = hex_to_string(election.epk_E, crypto_core_ristretto255_BYTES);
    auto entry = ballots.find(upk_str);
    bool election_exists = (entry != ballots.end());
    assert(election_exists);
    entry->second.push_back(ballot);
}

void Server::create_group(Group &group)
{
    VEP<BatchedDLEQProof> ve(NULL);

    // generation of group-specific key
    ve.gen(s_G, spk_G, spk_proof);

    // sending the key to the group
    memcpy(group.spk_G, spk_G, crypto_core_ristretto255_BYTES);
}

void Server::create_election(Election &election, int min_value, int max_value)
{
    VEP<BatchedDLEQProof> ve(NULL);

    // generation of group-specific key
    ve.gen(s_E, epk_E, epk_proof);

    // sending the key to the group
    memcpy(election.s_E, s_E, crypto_core_ristretto255_SCALARBYTES);
    memcpy(election.epk_E, epk_E, crypto_core_ristretto255_BYTES);

    election.min_value = min_value;
    election.max_value = max_value;

    auto upk_str = hex_to_string(election.epk_E, crypto_core_ristretto255_BYTES);
    bool election_exists = (ballots.find(upk_str) != ballots.end());
    assert(!election_exists);
    ballots.insert(BallotsMap::value_type(upk_str, BallotVector()));
}

void UserRegister(User &user, Server &server)
{
    // Generate random scalars v_i
    crypto_core_ristretto255_scalar_random(user.v_i);

    VEP<BatchedDLEQProof> ve(NULL);
    ve.gen(user.u_i, user.upk_u_i, user.upk_proof);

    std::cout << "User registered with public key: "
        << hex_to_string(user.upk_proof, user.upk_BYTES)
        << std::endl;

    // Simulate sending the public key to the server
    server.register_user(user);
}

void UserVote(User &voter, User &votee, int vote, Server &server)
{
    // Encode vote = x_{i,j} from domain D
    unsigned char x_i_j[crypto_core_ristretto255_SCALARBYTES]; // x_{i,j}
    memset(x_i_j, 0x00, crypto_core_ristretto255_SCALARBYTES);
    x_i_j[0] = (unsigned char)vote;         // TODO: How can I transpose this to Java?

    // Calculate y_{i,j} = (upk_{u_j})^{v_i} * g^{x_{i,j}}
    unsigned char upk_v[crypto_core_ristretto255_BYTES];  // Holds upk_{u_j}^{v_i}

    // Compute upk_{u_j}^{v_i}
    if (crypto_scalarmult_ristretto255(upk_v, voter.v_i, votee.upk_u_i) != 0) {
        printf("failed computing upk^v\n");
        abort();
    }

    // Compute g^{x_{i,j}}
    unsigned char g_x_i_j[crypto_core_ristretto255_BYTES];
    crypto_scalarmult_ristretto255_base(g_x_i_j, x_i_j);

    // Now multiply temp (upk_{u_j}^{v_i}) with g^{x_{i,j}}
    unsigned char *ballot = (unsigned char *)malloc(crypto_core_ristretto255_BYTES); // freed during server destruction
    crypto_core_ristretto255_add(ballot, upk_v, g_x_i_j);

    // Print the encrypted vote
    std::cout << "Vote: "
        << vote
        << "  Ballot: "
        << hex_to_string(ballot, crypto_core_ristretto255_BYTES)
        << std::endl;

    // Simulate sending the ballot
    server.add_ballot(votee, ballot);
}

void UserElectionVote(User &voter, Election &election, int vote, Server &server)
{
    // Encode vote = x_{i,j} from domain D
    unsigned char x_i_j[crypto_core_ristretto255_SCALARBYTES]; // x_{i,j}
    memset(x_i_j, 0x00, crypto_core_ristretto255_SCALARBYTES);
    x_i_j[0] = (unsigned char)vote;

    // Calculate y_{i,j} = (upk_{u_j})^{v_i} * g^{x_{i,j}}
    unsigned char epk_v[crypto_core_ristretto255_BYTES];  // Holds upk_{u_j}^{v_i}

    // Compute upk_{u_j}^{v_i}
    if (crypto_scalarmult_ristretto255(epk_v, voter.v_i, election.epk_E) != 0) {
        printf("failed computing epk^v\n");
        abort();
    }

    // Compute g^{x_{i,j}}
    unsigned char g_x_i_j[crypto_core_ristretto255_BYTES];
    crypto_scalarmult_ristretto255_base(g_x_i_j, x_i_j);

    // Now multiply temp (upk_{u_j}^{v_i}) with g^{x_{i,j}}
    unsigned char *ballot = (unsigned char *)malloc(crypto_core_ristretto255_BYTES); // freed during server destruction
    crypto_core_ristretto255_add(ballot, epk_v, g_x_i_j);

    // Print the encrypted vote
    std::cout << "Vote: "
        << vote
        << "  Ballot: "
        << hex_to_string(ballot, crypto_core_ristretto255_BYTES)
        << std::endl;

    // Simulate sending the ballot
    server.add_ballot(election, ballot);
}