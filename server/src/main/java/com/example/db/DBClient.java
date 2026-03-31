package com.example.db;

import com.example.models.Election;
import com.example.models.Intersection;
import com.example.models.Group;
import com.example.models.User;
import com.example.notifications.Notification;

import java.time.Instant;
import java.util.List;

public interface DBClient {

    void createUser(String upk);

    void createGroup(String spk, String s_G);

    Group getGroup(String spk);

    boolean groupExists(String spk);

    boolean isGroupInviteValid(String spk, String upk);

    void addUserToGroup(String upk, String spk, String tag);

    List<Group> getUserGroups(String upk);

    List<Notification> getUserMailbox(String upk);

    void addUserVote(String votee, String vote, String upk);

    void addElectionVote(String epk, String vote, String upk);

    void addBulkUserVotes(String upk, String... values);

    List<String> getUserVotes(String upk);

    List<String> getGroupTags(String spk);

    List<String> getGroupUsersUpks(String spk);

    List<User> getGroupUsers(String spk);

    boolean hasVoted(boolean isElection, String votee, String voter);

    void addEntry(String key, String... values);

    boolean exists(String key);

    List<String> getValues(String key);

    List<String> getAllValues(String key);

    int getNumValues(String key);

    int getNumAllValues(String key);

    void deleteEntry(String key);

    void close();

    void addIntersection(String key, Intersection intersection);

    Intersection getIntersection(String key);

    void updateIntersection(String key, Intersection intersection);

    void removeIntersection(String key);

    void addElection(String electionKey, Election election);

    Election getElection(String epk);

    List<Election> getElectionsForIntersection(Instant now);

    User getUser(String upk);

    void addBulkInvites(String spk, List<String> keys);
}
