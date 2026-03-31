package com.example.db;

import com.example.models.Intersection;
import com.example.models.Election;
import com.example.models.Group;
import com.example.models.User;
import com.example.notifications.Notification;

import java.time.Instant;
import java.time.LocalDateTime;
import java.time.ZoneId;
import java.time.format.DateTimeFormatter;
import java.util.*;
import java.util.concurrent.ConcurrentHashMap;

public class NewManager implements DBClient {

    private static NewManager instance;

    // Databases
    private final Map<String, User> users;
    private final Map<String, Group> groups;
    private final Map<String, Election> elections;
    private final Map<String, Intersection> intersections;

    // To have a singleton
    public static synchronized NewManager getInstance() {
        if (instance == null) {
            instance = new NewManager();
        }
        return instance;
    }

    private NewManager() {
        users = new ConcurrentHashMap<>();
        groups = new ConcurrentHashMap<>();
        elections = new ConcurrentHashMap<>();
        intersections = new ConcurrentHashMap<>();
    }

    @Override
    public synchronized void createUser(String upk) {
        if (users.get(upk) == null) users.put(upk, new User(upk));
    }

    @Override
    public synchronized void createGroup(String spk, String s_G) {
        if (groups.get(spk) == null) groups.put(spk, new Group(spk, s_G));
    }

    @Override
    public Group getGroup(String spk) {
        return groups.get(spk);
    }

    @Override
    public boolean groupExists(String spk) {
        return groups.containsKey(spk);
    }

    @Override
    public boolean isGroupInviteValid(String spk, String upk) {
        Group group = groups.get(spk);
        if (group != null) return group.isUserInvited(upk);
        else return false;
    }

    @Override
    public synchronized void addUserToGroup(String upk, String spk, String tag) {
        User user = users.get(upk);
        Group group = groups.get(spk);

        if (users.get(upk) != null && groups.get(spk) != null) {
            user.addGroup(group);
            group.addUser(user, tag);
        }
    }

    public User getUser(String upk) {
        return users.get(upk);
    }

    @Override
    public List<Group> getUserGroups(String upk) {
        User user = users.get(upk);
        if (user != null) return user.getGroups();
        return null;
    }

    @Override
    public synchronized void addBulkInvites(String spk, List<String> keys) {
        Group group = groups.get(spk);
        if (group != null) group.addBulkInvites(keys);
    }

    @Override
    public List<Notification> getUserMailbox(String upk) {
        User user = users.get(upk);
        if (user != null) return user.getMailbox();
        return null;
    }

    @Override
    public synchronized void addUserVote(String votee, String vote, String upk) {
        User user = users.get(votee);
        if (user != null) user.addVote(vote, upk);
    }

    @Override
    public synchronized void addElectionVote(String epk, String vote, String upk) {
        Election election = elections.get(epk);
        if (election != null) election.addVote(vote, upk);
    }

    @Override
    public synchronized void addBulkUserVotes(String upk, String... values) {
        User user = users.get(upk);
        if (user != null) user.addBulkVotes(values);
    }

    @Override
    public List<String> getUserVotes(String upk) {
        User user = users.get(upk);
        if (user != null) return user.getVotes();
        return null;
    }

    @Override
    public List<String> getGroupTags(String spk) {
        Group group = groups.get(spk);
        if (group != null) return group.getTags();
        return null;
    }

    @Override
    public List<String> getGroupUsersUpks(String spk) {
        Group group = groups.get(spk);
        if (group != null) return group.getUsersUpks();
        return new LinkedList<>();
    }

    @Override
    public List<User> getGroupUsers(String spk) {
        Group group = groups.get(spk);
        if (group != null) return group.getUsers();
        return null;
    }

    @Override
    public boolean hasVoted(boolean isElection, String votee, String voter) {
        if (isElection && elections.get(votee).getVoters().contains(voter)) {
            return true;
        }
        else if (!isElection && users.get(votee).getVoters().contains(voter)) {
            return true;
        }
        return false;
    }


    // --------------------

    @Override
    public void addEntry(String key, String... values) {

    }

    @Override
    public boolean exists(String key) {
        return false;
    }

    @Override
    public List<String> getValues(String key) {
        return null;
    }

    @Override
    public List<String> getAllValues(String key) {
        return null;
    }

    @Override
    public int getNumValues(String key) {
        return 0;
    }

    @Override
    public int getNumAllValues(String key) {
        return 0;
    }

    @Override
    public void deleteEntry(String key) {

    }

    @Override
    public void close() {

    }

    @Override
    public void addIntersection(String key, Intersection intersection) {
        intersections.putIfAbsent(key, intersection);
    }

    @Override
    public Intersection getIntersection(String key) {
        return intersections.get(key);
    }

    @Override
    public void updateIntersection(String key, Intersection intersection) {
        intersections.put(key, intersection);
    }

    @Override
    public void removeIntersection(String key) {
        intersections.remove(key);
    }

    @Override
    public void addElection(String electionKey, Election election) {
        elections.putIfAbsent(electionKey, election);
        System.out.println("Created election with key: " + electionKey);
    }

    @Override
    public Election getElection(String epk) {
        return elections.get(epk);
    }

    @Override
    public List<Election> getElectionsForIntersection(Instant now) {
        List<Election> filteredElections = new ArrayList<>();
        for (Election e : elections.values()) {

            String deadlineStr = e.getDeadline();
            Instant deadline;

            try {
                // Case 1: ISO string with timezone ("Z" or offset)
                deadline = Instant.parse(deadlineStr);
            } catch (Exception ex) {
                // Case 2: Fallback to local date-time (no zone/offset info)
                LocalDateTime ldt = LocalDateTime.parse(deadlineStr, DateTimeFormatter.ISO_LOCAL_DATE_TIME);
                deadline = ldt.atZone(ZoneId.systemDefault()).toInstant();
            }

            if (!e.isDone()) {
                if (now.isAfter(deadline)) {
                    System.out.println("Deadline has passed");
                    filteredElections.add(e);
                } else {
                    System.out.println("Deadline is still in the future");
                }
            }


        }

        return filteredElections;
    }
}
