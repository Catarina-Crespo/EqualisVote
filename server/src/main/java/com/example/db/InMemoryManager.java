package com.example.db;

import com.example.models.Election;
import com.example.models.Intersection;
import com.example.models.Group;
import com.example.models.User;
import com.example.notifications.Notification;

import java.time.Instant;
import java.time.LocalDateTime;
import java.time.ZoneId;
import java.time.format.DateTimeFormatter;
import java.util.*;

public class InMemoryManager implements DBClient {

    private static InMemoryManager instance;
    private final Map<String, List<String>> db;
    private final Map<String, Intersection> intersectionsMap;
    private final Map<String, Election> electionsMap;

    private InMemoryManager() {
        db = new HashMap<>();
        intersectionsMap = new HashMap<>();
        electionsMap = new HashMap<>();
    }

    public static synchronized InMemoryManager getInstance() {
        if (instance == null) {
            instance = new InMemoryManager();
        }
        return instance;
    }

    @Override
    public void createUser(String upk) {

    }

    @Override
    public void createGroup(String spk, String s_G) {

    }

    @Override
    public Group getGroup(String spk) {
        return null;
    }

    @Override
    public boolean groupExists(String spk) {
        return false;
    }

    @Override
    public boolean isGroupInviteValid(String spk, String upk) {
        return false;
    }

    @Override
    public void addUserToGroup(String upk, String spk, String tag) {

    }

    @Override
    public List<Group> getUserGroups(String upk) {
        return null;
    }

    @Override
    public List<Notification> getUserMailbox(String upk) {
        return null;
    }

    @Override
    public void addUserVote(String votee, String vote, String upk) {

    }

    @Override
    public void addElectionVote(String epk, String vote, String upk) {

    }

    @Override
    public void addBulkUserVotes(String upk, String... values) {

    }

    @Override
    public List<String> getUserVotes(String upk) {
        return null;
    }

    @Override
    public List<String> getGroupTags(String spk) {
        return null;
    }

    @Override
    public List<String> getGroupUsersUpks(String spk) {
        return null;
    }

    @Override
    public List<User> getGroupUsers(String spk) {
        return null;
    }

    @Override
    public boolean hasVoted(boolean isElection, String votee, String voter) {
        return false;
    }

    @Override
    public void addEntry(String key, String... values) {
        db.computeIfAbsent(key, k -> new ArrayList<>()).addAll(Arrays.asList(values));
    }

    @Override
    public boolean exists(String key) {
        return db.containsKey(key) || intersectionsMap.containsKey(key) || electionsMap.containsKey(key);
    }

    @Override
    public List<String> getValues(String key) {
        List<String> list = db.get(key);
        if (list != null) return list.subList(1, list.size()); else return null;
    }

    @Override
    public List<String> getAllValues(String key) {
        List<String> list = db.get(key);
        if (list != null) return list.subList(0, list.size()); else return null;
    }

    @Override
    public int getNumValues(String key) {
        List<String> list = db.get(key);
        if (list != null) return list.size() - 1; else return 0;
    }

    @Override
    public int getNumAllValues(String key) {
        List<String> list = db.get(key);
        if (list != null) return list.size(); else return 0;
    }

    @Override
    public void deleteEntry(String key) {
        db.remove(key);
    }

    @Override
    public void close() {

    }

    @Override
    public void addIntersection(String key, Intersection intersection) {
        intersectionsMap.put(key, intersection);
    }

    @Override
    public Intersection getIntersection(String key) {
        return intersectionsMap.get(key);
    }

    @Override
    public void updateIntersection(String key, Intersection intersection) {
        intersectionsMap.put(key, intersection);
    }

    @Override
    public void removeIntersection(String key) {
        intersectionsMap.remove(key);
    }

    @Override
    public void addElection(String epk, Election election) {
        electionsMap.put(epk, election);
    }

    @Override
    public Election getElection(String epk) {
        return electionsMap.get(epk);
    }

    @Override
    public List<Election> getElectionsForIntersection(Instant now) {
        List<Election> elections = new ArrayList<>();
        for (Election e : electionsMap.values()) {
            //Instant deadline = Instant.parse(e.getDeadline());
            LocalDateTime ldt = LocalDateTime.parse(e.getDeadline(), DateTimeFormatter.ISO_LOCAL_DATE_TIME);
            Instant deadline = ldt.atZone(ZoneId.systemDefault()).toInstant();
            if (now.isAfter(deadline)) {
                System.out.println("Deadline has passed");
                elections.add(e);
            } else {
                System.out.println("Deadline is still in the future");
            }
        }

        return elections;
    }

    @Override
    public User getUser(String upk) {
        return null;
    }

    @Override
    public void addBulkInvites(String spk, List<String> keys) {

    }

}
