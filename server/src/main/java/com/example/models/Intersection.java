package com.example.models;

import com.example.ShuffleNative;

import java.util.Arrays;

import static com.example.Utils.*;

public class Intersection {

    private String externalUserUPK;
    private String groupUserUPK;

    private String T;
    private String Tprime;
    private String Tpp;
    private String s_i_star_G;
    private String overline_spk_i_start_G;
    private String overline_spk_proof;
    private String Delta_i_star_G;
    private String spk_i_star_G;
    private String spk_i_star_proof;
    private String rho_i_star_G;

    private String Ws;

    private int nBallots;

    public Intersection() {
    }

    /**
     * Calls the C++ module to initialize the intersection for this election and
     * to generate Tprime from T, exponentiate the ballots to Ws and generate
     * overline_spk_i_start_G
     *
     * @param T - list of original user-group tags in hexadecimal format
     * @param groupSize - size of the group
     * @param s_G - group secret key
     * @param externalUserUPK - not used
     * @param nBallots - number of votes cast for this election/user
     * @param ballots - string of ballots in hexadecimal format
     */
    public void shuffleT(String T, int groupSize, String s_G, String externalUserUPK, int nBallots, String ballots) {

        System.out.println("About to create the Shuffle Native");

        ShuffleNative nativeLib = new ShuffleNative();

        String[] TArray = T.split(" ");
        String[] ballotArray = ballots.split(" ");

        System.out.println("Calling the shuffle native:");

        ShuffleResult r = nativeLib.shuffleT(
                groupSize,
                s_G,
                TArray,
                nBallots,
                ballotArray
        );

        System.out.println("Produced Tprime: " + r.Tprime);
        System.out.println("Produced Ws: " + r.Ws);
        System.out.println("Produced overline: " + r.overline_spk_i_start_G);

        Tprime = r.Tprime.trim();
        Ws = r.Ws.trim();
        overline_spk_i_start_G = r.overline_spk_i_start_G.trim();
        this.nBallots = nBallots;
    }

    public String getTprime() {
        return Tprime;
    }

    public String getExternalUserUPK() {
        return externalUserUPK;
    }

    public String getGroupUserUPK() {
        return groupUserUPK;
    }

    public String getWs() {
        return Ws;
    }

    public String getOverline_spk_i_start_G() {
        return overline_spk_i_start_G;
    }

    public int getNBallots() {
        return nBallots;
    }

    public boolean acceptUser(String votes) {
        int[] parsedVotes = convertIntoIntArray(votes);

        double average = (double) Arrays.stream(parsedVotes).sum() / parsedVotes.length;
        System.out.println("Printing calculated score: " + average);


        System.out.println("Printing calculated score: " + average);

        // Some way of calculating
        return average >= 5.0;
    }

    public int[] storeElectionResult(String votes, Election election) {
        int[] parsedVotes = convertIntoIntArray(votes);

        int[] result = new int[election.getOptions().length];

        for (int i = 0; i < parsedVotes.length; i++) {
            result[parsedVotes[i]]++;
        }
        election.setResult(result);
        election.setDone();

        // Some way of calculating
        return result;
    }

    public void setGroupUserUPK(String groupUserUPK) {
        this.groupUserUPK = groupUserUPK;
    }

    public void setExternalUserUPK(String externalUserUPK) {
        this.externalUserUPK = externalUserUPK;
    }
}
