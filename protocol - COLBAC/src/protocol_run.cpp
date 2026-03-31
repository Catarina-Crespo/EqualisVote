#include "protocol_run.hpp"
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

protocol_measurements protocol_run(
    // c. Function arguments
    size_t initial_group_size,
    size_t n_voting_users,
    int min_vote,               // c. minimum possible value for score
    int max_vote                // c. maximum possible value for score
)
{
    protocol_measurements measurements;     // c. variable to store the measurements obtained
    measurements.full_protocol_BYTES = 0;   // c. TODO: idk what is this for - I guess it's initializing this variable??
    auto pre_join_start = std::chrono::high_resolution_clock::now();    // c. starts the clock

    // c. verifies if variables respect the constraints
    assert(initial_group_size > 0);
    assert(min_vote >= 0);
    assert(max_vote <= 255);
    assert(min_vote < max_vote);

    std::cout << "--- PROTOCOL RUN ---" << std::endl;

    // Create a group and a user
    Server server;
    Group group;
    std::vector<User> users;    // c. List to store users
    User external_user;

    // prepare DLOG proof engine
    DLOG2GenProof dlog2gen_pf;

    // Simulate user registration
    std::cout << "U.Register..." << std::endl;

    // c. Creates some users in the group and some outside; TODO: I don't remember the reason
    for (auto i = 0; i < initial_group_size + n_voting_users/2; i++)
    {
        User user;                              // c. Creates an object of type User
        UserRegister(user, server);       // c. Registers the user on the server
        assert(dlog2gen_pf.verify(user.upk_u_i, user.upk_proof));   // c. TODO: Verifies the user public key created on UserRegister
        users.push_back(user);                  // c. Adds the user to the list of users
        // c. TODO: Users' keys are sent to the server on UserRegister, but they are not sent to the group or all the users, right?
        // Even though it is in the protocol
    }

    // c. Same thing for the external user trying to join
    UserRegister(external_user, server);
    assert(dlog2gen_pf.verify(external_user.upk_u_i, external_user.upk_proof));
    // count the public key and proof from the external to the group
    measurements.full_protocol_BYTES += dlog2gen_pf.statement_BYTES + dlog2gen_pf.proof_BYTES; // c. TODO: Idk what this is for
    
    // Simulate users voting
    std::vector<int> sent_votes;            // c. TODO: What's the diference between the sent_votes and group_votes
    std::vector<int> group_votes;
    std::cout << "U.Vote..." << std::endl;
    for (auto i = 0; i < n_voting_users; i++)
    {
        uint32_t rand_int = randombytes_random() % (max_vote - min_vote + 1); // c. Generates a random int for score
        rand_int += min_vote;
        int vote = (int)rand_int;
        sent_votes.push_back(vote);             // c. Adds the vote to the sent votes
        if (i >= n_voting_users/2) {            // c. Half of the votes are stored in the groups' votes list
            group_votes.push_back(vote);
        }
        UserVote(users[i], external_user, vote, server);
    }

    // Simulate group creation and user joining
    std::cout << "S.CreateGroup..." << std::endl;
    server.create_group(group);     // c. Creates the group on the server and generates the keys
    assert(dlog2gen_pf.verify(server.spk_G, server.spk_proof));

    // unconditionally add initial group members
    std::cout << "U.JoinGroup..." << std::endl;
    for (auto i = n_voting_users/2; i < initial_group_size + n_voting_users/2 ; i++)
    {
        users[i].join_group(group);     // c. Includes sending the key generated for the group to the user
    }
    auto pre_join_end = std::chrono::high_resolution_clock::now();

    std::cout << "External user initiates group-join request" << std::endl;
    auto join_start = std::chrono::high_resolution_clock::now();

    // group size
    size_t ell = group.user_tokens.size();
    std::cout << "Group size: " << ell << std::endl;

    // ------------- Join begin (server) -------------

    // VE engine
    VEP<BatchedDLEQProof> ve(NULL);     // c. TODO: This initializes the VE engine?

    // ------------- Join begin (user) -------------

    // U.InitCount - TODO: The external user initializes the permutation??
    std::cout << "U.InitCount..." << std::endl;
    size_t sigma_i_star_G[ell];             // c. TODO: This creates a variable sigma_i_star_G to store a permutation
    random_permutation(ell, sigma_i_star_G);
    // user key and proof to group
    measurements.full_protocol_BYTES += dlog2gen_pf.statement_BYTES + dlog2gen_pf.proof_BYTES;

    // ------------- Join begin (server initializations) -------------

    // S.InitCount
    std::cout << "S.InitCount..." << std::endl;
    size_t rho_i_star_G[ell];
    random_permutation(ell, rho_i_star_G);
    unsigned char s_i_star_G[ve.secret_key_BYTES];
    unsigned char overline_spk_i_start_G[ve.public_key_BYTES];
    unsigned char overline_spk_proof[ve.key_proof_BYTES];
    ve.gen(s_i_star_G, overline_spk_i_start_G, overline_spk_proof);

    unsigned char inv_s_G[crypto_core_ristretto255_SCALARBYTES];
    if (crypto_core_ristretto255_scalar_invert(inv_s_G, server.s_G) != 0)
    {
        printf("failed inverting s_G\n");
        abort();
    }
    unsigned char Delta_i_star_G[crypto_core_ristretto255_SCALARBYTES];
    unsigned char spk_i_star_G[crypto_core_ristretto255_BYTES];
    unsigned char spk_i_star_proof[DLOGProof::proof_BYTES];
    crypto_core_ristretto255_scalar_mul(Delta_i_star_G, s_i_star_G, inv_s_G);
    crypto_scalarmult_ristretto255_base(spk_i_star_G, Delta_i_star_G); // spk_{i^*, G} = g^{Delta_{i^*, G}}
    dlog2gen_pf.prove(Delta_i_star_G, spk_i_star_G, spk_i_star_proof);

    assert(dlog2gen_pf.verify(spk_i_star_G, spk_i_star_proof));
    measurements.full_protocol_BYTES += dlog2gen_pf.statement_BYTES + dlog2gen_pf.proof_BYTES;

    assert(dlog2gen_pf.verify(overline_spk_i_start_G, overline_spk_proof));
    measurements.full_protocol_BYTES += dlog2gen_pf.statement_BYTES + dlog2gen_pf.proof_BYTES;

    // G.InitExp
    std::cout << "G.InitExp..." << std::endl;

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
    unsigned char *T[ell];
    for (auto i = 0; i < ell; i++)
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

    // S.ShuffleExp
    std::cout << "S.ShuffleExp..." << std::endl;

    // c. Creates an array of pointers, where each Tprime[i] is a pointer to a raw byte array
    // c. The code here is allocating memory (crypto_core_ristretto255_BYTES) for each array
    unsigned char *Tprime[ell];
    for (auto i = 0; i < ell; i++) {
        Tprime[i] = (unsigned char *) malloc(crypto_core_ristretto255_BYTES);
    }

    // perform VPE evaluation - .TODO: I don't understand this part - this only creates instances of these things
    ShuffledSigmaProtocol<SCDLEQSigmaProtocol> pfsys(ell);
    RepeatedSigmaProtocol<ShuffledSigmaProtocol<SCDLEQSigmaProtocol>> repfsys(128, pfsys);
    FiatShamir<RepeatedSigmaProtocol<ShuffledSigmaProtocol<SCDLEQSigmaProtocol>>> fsproof(repfsys);
    VEP<FiatShamir<RepeatedSigmaProtocol<ShuffledSigmaProtocol<SCDLEQSigmaProtocol>>>> vep(&fsproof);

    auto server_vep_pf_start = std::chrono::high_resolution_clock::now();
    unsigned char S_shuffle_exp_proof[vep.exponentiation_proof_BYTES];
    if (vep.eval(
        Delta_i_star_G, spk_i_star_G,
        ell, T,
        rho_i_star_G,
        Tprime,
        S_shuffle_exp_proof
    ) != 0) {
        assert(false);
    }
    auto server_vep_pf_end = std::chrono::high_resolution_clock::now();
    measurements.server_vep_input_BYTES = ell * crypto_core_ristretto255_BYTES;
    measurements.server_vep_statement_BYTES = ell * crypto_core_ristretto255_BYTES;
    measurements.server_vep_proof_BYTES = vep.exponentiation_proof_BYTES;
    measurements.full_protocol_BYTES += measurements.server_vep_input_BYTES
                                        + measurements.server_vep_statement_BYTES
                                        + measurements.server_vep_proof_BYTES;


    // check proof
    auto server_vep_vf_start = std::chrono::high_resolution_clock::now();
    assert(vep.check(spk_i_star_G, ell, T, Tprime, S_shuffle_exp_proof));
    auto server_vep_vf_end = std::chrono::high_resolution_clock::now();

    // ------------- Join (server shuffling end) -------------

    // c. After the server has shuffled the tags, the user does it as well

    // ------------- Join (user shuffling) -------------

    // U.ShuffleExp
    std::cout << "U.ShuffleExp..." << std::endl;
    unsigned char *Tpp[ell];
    for (auto i = 0; i < ell; i++) {
        Tpp[i] = (unsigned char *) malloc(crypto_core_ristretto255_BYTES);
    }

    auto user_vep_pf_start = std::chrono::high_resolution_clock::now();
    unsigned char U_shuffle_exp_proof[vep.exponentiation_proof_BYTES];
    if (vep.eval(
        external_user.u_i, external_user.upk_u_i,
        ell, Tprime,
        sigma_i_star_G,
        Tpp,
        U_shuffle_exp_proof
    ) != 0) {
        assert(false);
    }
    auto user_vep_pf_end = std::chrono::high_resolution_clock::now();
    measurements.user_vep_input_BYTES = ell * crypto_core_ristretto255_BYTES;
    measurements.user_vep_statement_BYTES = ell * crypto_core_ristretto255_BYTES;
    measurements.user_vep_proof_BYTES = vep.exponentiation_proof_BYTES;
    measurements.full_protocol_BYTES += measurements.server_vep_input_BYTES
                                        + measurements.server_vep_statement_BYTES
                                        + measurements.server_vep_proof_BYTES;

    // check proof
    auto user_vep_vf_start = std::chrono::high_resolution_clock::now();
    assert(vep.check(external_user.upk_u_i, ell, Tprime, Tpp, U_shuffle_exp_proof));
    auto user_vep_vf_end = std::chrono::high_resolution_clock::now();

    // ------------- Join (user shuffling end) -------------

    // ------------- User must then send the resulting ballots to the server -------------

    // S.SendVotes
    std::cout << "U/S.SendVotes..." << std::endl;

    // recover ballots from external user
    auto upk_i_star_str = hex_to_string(external_user.upk_u_i, external_user.upk_BYTES);
    auto entry = server.ballots.find(upk_i_star_str);       // c. Recover the entry from the server related with i_star (external user)
    bool user_exists = (entry != server.ballots.end());
    assert(user_exists);
    BallotVector vec_of_ballots = entry->second;            // c. Recover the vector of ballots
    size_t n_ballots = vec_of_ballots.size();
    std::cout << "Ballots present: " << n_ballots << std::endl;
    measurements.ballots_BYTES = n_ballots * crypto_core_ristretto255_BYTES;
    measurements.full_protocol_BYTES += measurements.ballots_BYTES;

    // extract ballots and prepare for server's VE
    unsigned char *W[n_ballots];
    unsigned char *W_s[n_ballots];
    // c. Allocate memory for the ballots
    for (auto i = 0; i < n_ballots; i++) {
        W[i] = (unsigned char *) malloc(crypto_core_ristretto255_BYTES);
        W_s[i] = (unsigned char *) malloc(crypto_core_ristretto255_BYTES);
    }
    ctr = 0;
    // c. Copy the ballots stored by the external user into W - .TODO: why? I guess to not modify the original, I'm not sure what Eval does, maybe it changes the list of votes
    for(const auto& ballot : vec_of_ballots) 
    {
        memcpy(W[ctr], ballot, crypto_core_ristretto255_BYTES);
        ctr++;
    }

    // ------------- New shuffling (server start) -------------

    // perform Server's VE on ballots
    auto ve_pf_start = std::chrono::high_resolution_clock::now();
    unsigned char S_send_votes_proof[ve.exponentiation_proof_BYTES];
    if (ve.eval(
        s_i_star_G, overline_spk_i_start_G,
        n_ballots, W,
        NULL,
        W_s,
        S_send_votes_proof
    ) != 0) {
        assert(false);
    }
    auto ve_pf_end = std::chrono::high_resolution_clock::now();
    measurements.ve_statement_BYTES = 2 * n_ballots * crypto_core_ristretto255_BYTES;
    measurements.ve_proof_BYTES = ve.exponentiation_proof_BYTES;
    measurements.full_protocol_BYTES += measurements.ve_statement_BYTES
                                        + measurements.ve_proof_BYTES;

    // check proof
    auto ve_vf_start = std::chrono::high_resolution_clock::now();
    assert(ve.check(
        overline_spk_i_start_G,
        n_ballots,
        W,
        W_s,
        S_send_votes_proof
    ));
    auto ve_vf_end = std::chrono::high_resolution_clock::now();

    // ------------- Intersection (server start) -------------

    // G.IntersectVotes
    std::cout << "G.IntersectVotes..." << std::endl;
    auto intersect_start = std::chrono::high_resolution_clock::now();
    unsigned char inv_alpha[crypto_core_ristretto255_SCALARBYTES];
    crypto_core_ristretto255_scalar_invert(inv_alpha, alpha);
    unsigned char *T3p[ell];

    // c. Exponentiates Tpp to the inverse of alpha
    for (auto i = 0; i < ell; i++)
    {
        T3p[i] = (unsigned char *)malloc(crypto_core_ristretto255_BYTES);
        if (crypto_scalarmult_ristretto255(T3p[i], inv_alpha, Tpp[i]) != 0)
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
    for (auto y_indx = 0; y_indx < ell; y_indx++)
    {
        // c. Takes each encrypted ballot
        for (auto w_indx = 0; w_indx < n_ballots; w_indx++)
        {
            // c. For each possible value for a vote
            for (int candidate_vote = min_vote; candidate_vote <= max_vote; candidate_vote++)
            {
                encoded_candidate_vote[0] = (unsigned char)candidate_vote;
                crypto_core_ristretto255_add(yw, T3p[y_indx], W_s[w_indx]); // yw = T3p * W_s
                // due to a quirk in crypto_scalarmult_ristretto255, the 0-vote is handled separatedly
                if (candidate_vote == 0) {
                    ristretto255_identity_point(recovered_yw);
                } else {
                    // c. Compute b = a^k, crypto_scalarmult_ristretto255(b, k, a) != 0
                    if (crypto_scalarmult_ristretto255(recovered_yw, encoded_candidate_vote, overline_spk_i_start_G) != 0)
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
    auto intersect_end = std::chrono::high_resolution_clock::now();

    // ------------- Intersection (server end) -------------

    // print the recovered votes
    std::cout << "     Sent votes: ";
    for(const auto& vote : sent_votes)
    {
        std::cout << vote << " ";
    }
    std::cout << std::endl;

    std::cout << "    Group votes: ";
    for(const auto& vote : group_votes)
    {
        std::cout << vote << " ";
    }
    std::cout << std::endl;

    std::cout << "Recovered votes: ";
    for(const auto& rec_vote : X)
    {
        std::cout << rec_vote << " ";
    }
    std::cout << std::endl;

    // check for exact match in votes
    std::sort(X.begin(), X.end());
    std::sort(group_votes.begin(), group_votes.end());
    assert(group_votes == X);
    std::cout << "(Exactly) the group votes were recovered successfully." << std::endl;
    
    // recover applied permutation
    // size_t perm[ell];
    // compose_permutations(ell, rho_i_star_G, sigma_i_star_G, perm);
    // std::cout << "Overall VPE permutation: ";
    // print_permutation(ell, perm);

    // admit external user to group
    std::cout << "Admitting external into group" << std::endl;
    external_user.join_group(group);
    measurements.full_protocol_BYTES += crypto_core_ristretto255_BYTES; // the user tag
    measurements.full_protocol_BYTES += ell * crypto_core_ristretto255_BYTES; // the tags already in the group

    auto join_end = std::chrono::high_resolution_clock::now();

    std::cout << "--- PROTOCOL END ---" << std::endl;

    auto ve_pf_duration = ve_pf_end - ve_pf_start;
    auto ve_vf_duration = ve_vf_end - ve_vf_start;
    auto user_vep_pf_duration = user_vep_pf_end - user_vep_pf_start;
    auto user_vep_vf_duration = user_vep_vf_end - user_vep_vf_start;
    auto server_vep_pf_duration = server_vep_pf_end - server_vep_pf_start;
    auto server_vep_vf_duration = server_vep_vf_end - server_vep_vf_start;
    auto intersect_duration = intersect_end - intersect_start;
    auto pre_join_duration = pre_join_end - pre_join_start;
    auto join_duration = join_end - join_start;

    measurements.ve_prove = std::chrono::duration_cast<std::chrono::milliseconds>(ve_pf_duration).count();
    measurements.ve_verify = std::chrono::duration_cast<std::chrono::milliseconds>(ve_vf_duration).count();
    measurements.user_vep_prove = std::chrono::duration_cast<std::chrono::milliseconds>(user_vep_pf_duration).count();
    measurements.user_vep_verify = std::chrono::duration_cast<std::chrono::milliseconds>(user_vep_vf_duration).count();
    measurements.server_vep_prove = std::chrono::duration_cast<std::chrono::milliseconds>(server_vep_pf_duration).count();
    measurements.server_vep_verify = std::chrono::duration_cast<std::chrono::milliseconds>(server_vep_vf_duration).count();
    measurements.intersection = std::chrono::duration_cast<std::chrono::milliseconds>(intersect_duration).count();
    measurements.pre_protocol = std::chrono::duration_cast<std::chrono::milliseconds>(pre_join_duration).count();
    measurements.full_protocol = std::chrono::duration_cast<std::chrono::milliseconds>(join_duration).count();

    for (auto i = 0; i < ell; i++)
    {
        free(T[i]);
        free(Tprime[i]);
        free(Tpp[i]);
        free(T3p[i]);
    }
    for (auto i = 0; i < n_voting_users; i++)
    {
        free(W[i]);
        free(W_s[i]);
    }
    return measurements;
}


void print_measurement(protocol_measurements measurements)
{
    std::cerr << "  {" << std::endl;
    std::cerr << "  \"runtime\": {" << std::endl;
    std::cerr << "    \"ve_prove\": " << measurements.ve_prove << "," << std::endl;
    std::cerr << "    \"ve_verify\": " << measurements.ve_verify << "," << std::endl;
    std::cerr << "    \"user_vep_prove\": " << measurements.user_vep_prove << "," << std::endl;
    std::cerr << "    \"user_vep_verify\": " << measurements.user_vep_verify << "," << std::endl;
    std::cerr << "    \"server_vep_prove\": " << measurements.server_vep_prove << "," << std::endl;
    std::cerr << "    \"server_vep_verify\": " << measurements.server_vep_verify << "," << std::endl;
    std::cerr << "    \"intersection\": " << measurements.intersection << "," << std::endl;
    std::cerr << "    \"pre_protocol\": " << measurements.pre_protocol << "," << std::endl;
    std::cerr << "    \"full_protocol\": " << measurements.full_protocol << std::endl;
    std::cerr << "  }," << std::endl;
    std::cerr << "  \"bandwidth\": {" << std::endl;
    std::cerr << "    \"ve_statement\": " << measurements.ve_statement_BYTES << "," << std::endl;
    std::cerr << "    \"ve_proof\": " << measurements.ve_proof_BYTES << "," << std::endl;
    std::cerr << "    \"user_vep_input\": " << measurements.user_vep_input_BYTES << "," << std::endl;
    std::cerr << "    \"user_vep_statement\": " << measurements.user_vep_statement_BYTES << "," << std::endl;
    std::cerr << "    \"user_vep_proof\": " << measurements.user_vep_proof_BYTES << "," << std::endl;
    std::cerr << "    \"server_vep_input\": " << measurements.server_vep_input_BYTES << "," << std::endl;
    std::cerr << "    \"server_vep_statement\": " << measurements.server_vep_statement_BYTES << "," << std::endl;
    std::cerr << "    \"server_vep_proof\": " << measurements.server_vep_proof_BYTES << "," << std::endl;
    std::cerr << "    \"ballots\": " << measurements.ballots_BYTES << "," << std::endl;
    std::cerr << "    \"full_protocol\": " << measurements.full_protocol_BYTES << std::endl;
    std::cerr << "  }" << std::endl;
    std::cerr << "  }" << std::endl;
}