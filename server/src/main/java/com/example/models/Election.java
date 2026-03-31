package com.example.models;

import java.util.Arrays;
import java.util.LinkedList;
import java.util.List;

public class Election {
    private String epk_E;
    private String name;
    private String[] options;
    private String deadline;
    private String group_spk;
    private int[] result;
    private List<String> votes;
    private boolean done;
    private List<String> voters;

    public Election() {
        this.votes = new LinkedList<>();
        this.voters = new LinkedList<>();
        this.done = false;
    }

    // Getters and setters
    public String getEpk_E() {
        return epk_E;
    }

    public void setEpk_E(String epk_E) {
        this.epk_E = epk_E;
    }

    public String getName() {
        return name;
    }

    public void setName(String name) {
        this.name = name;
    }

    public String[] getOptions() {
        return options;
    }

    public void setOptions(String[] options) {
        this.options = options;
    }

    public String getDeadline() {
        return deadline;
    }

    public void setDeadline(String deadline) {
        this.deadline = deadline;
    }

    public String getGroup_spk() {
        return group_spk;
    }

    public void setGroup_spk(String group_spk) {
        this.group_spk = group_spk;
    }

    public String getResult() {
        return Arrays.toString(result);

    }

    public void setResult(int[] result) {
        this.result = result;
    }

    public List<String> getVotes() { return votes; }

    public void addVote(String vote, String upk) {
        if (!votes.contains(vote)) {
            votes.add(vote);
            voters.add(upk);
        }
    }

    public void removeVote(String vote) { votes.remove(vote); }

    public boolean isDone() {
        return done;
    }

    public void setDone() {
        done = true;
    }

    public List<String> getVoters() { return voters; }
}
