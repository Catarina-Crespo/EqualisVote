package com.example.models;

import java.util.List;

import static com.example.Utils.*;

public class GroupDTO {
    public String spk;
    public String name;
    public final List<UserDTO> users;
    public final List<String> invitedUsers;
    public final List<ElectionDTO> elections;

    public GroupDTO(String spk, String groupName, List<User> users, List<String> invitedUsers, List<Election> elections) {
        this.spk = spk;
        this.name = groupName;
        this.users = convertUsersToDTO(users);
        this.invitedUsers = invitedUsers;
        this.elections = convertElectionsToDTO(elections);
    }

    public GroupDTO(Group group) {
        this.spk = group.getSpk();
        this.name = group.getGroupName();
        this.users = convertUsersToDTO(group.getUsers());
        this.invitedUsers = group.getInvitedUsers();
        this.elections = convertElectionsToDTO(group.getElections());
    }




}
