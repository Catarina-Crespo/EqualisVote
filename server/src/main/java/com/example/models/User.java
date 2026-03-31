package com.example.models;

import com.example.notifications.Notification;

import java.util.*;

public class User {

    private final String upk;
    private String username;
    private final List<Group> groups;
    private final List<String> votes;     // Votes cast by others on this user
    private final List<String> voters;
    //private final List<Notification> mailbox;
    private HashMap<String, Notification> mailbox;

    public User(String upk) {
        this.upk = upk;
        this.username = null;
        this.groups = new LinkedList<>();
        this.voters = new LinkedList<>();
        //this.mailbox = new LinkedList<>();
        this.mailbox = new HashMap<>();
        this.votes = new LinkedList<>();
    }

    public User(String upk, String username) {
        this.upk = upk;
        this.username = username;
        this.groups = new LinkedList<>();
        this.voters = new LinkedList<>();
        //this.mailbox = new LinkedList<>();
        this.mailbox = new HashMap<>();
        this.votes = new LinkedList<>();
    }

    public String getUpk() {
        return upk;
    }

    public void setUsername(String username) { this.username = username; }

    public String getUsername() { return username; }

    public List<Group> getGroups() {
        return groups;
    }

    public void addGroup(Group group) {
        if (!groups.contains(group)) groups.add(group);
    }

    public void removeGroup(Group group) {
        groups.remove(group);
    }

    public List<Notification> getMailbox() {
        return new ArrayList<>(mailbox.values());
    }

    public void addNotification(String ID, Notification mail) {
        if (!mailbox.containsKey(ID)) mailbox.put(ID, mail);
    }

    public void removeNotification(String ID) {
        mailbox.remove(ID);
        System.out.println("Removed notification with ID: " + ID);
    }

    public void addVote(String vote, String upk) {

        votes.add(vote);
        voters.add(upk);
    }

    public List<String> getVotes() {
        return votes;
    }

    public List<String> getVoters() {
        return voters;
    }

    public void addBulkVotes(String... votes) {
        this.votes.addAll(Arrays.asList(votes));
    }
}
