import React, { useState, useEffect } from 'react';
import { Link } from 'react-router-dom';
import QuickActions from '../components/QuickActions';
import { Icon } from "@iconify/react";
import { useUser } from '../context/UserContext';
import Group from '../components/Group';
import { getGroups, getNotifications } from '../backend/wasmAPI';
import Notification from '../components/Notification';

const Home = () => {

  const { keys } = useUser();

  const [groups, setGroups] = useState([]);
  const [notifications, setNotifications] = useState([]);
  //const [isCreateGroupModalOpen, setIsCreateGroupModalOpen] = useState(false);
  //const [isJoinGroupModalOpen, setIsJoinGroupModalOpen] = useState(false);

  const handleVoteSubmit = (voteData) => {
    console.log("Vote submitted:", voteData);
    // TODO: send to backend with axios.post("/send-vote", voteData)
  };

  const handleRefresh = () => {
    console.log("Called refresh")
    fetchGroups();
    fetchNotifications();
  }

  const fetchGroups = async () => {
    try {
      const result = await getGroups(keys.upk);
      console.log("Logging result:")
      console.log(result);

      if (result.size > 0 && result.groups) {
        setGroups(result.groups);
      }

      console.log("Fetched groups:", result.size);

    } catch (error) {
      console.error("Error fetching groups:", error);
    }
  };

  // NEW
  useEffect(() => {
    if (!keys) return; // wait until keys are loaded
    fetchGroups();
    fetchNotifications();
  }, [keys]); // re-run when keys change


  // ------------- NOTIFICATION
  const fetchNotifications = async () => {
    if (!keys) return;

    try {
      const result = await getNotifications(keys.upk);

      if (result.size > 0 && result.notifications) {
        const normalized = result.notifications.map((notifStr) => {
          let notif = notifStr;
          if (typeof notifStr === "string") {
            try {
              notif = JSON.parse(notifStr);
            } catch (err) {
              console.error("Invalid JSON:", notifStr, err);
            }
          }

          let data = notif.data || {};
          if (!notif.data) {
            Object.keys(notif).forEach((k) => {
              if (k !== "type") data[k] = notif[k];
            });
          }

          return {
            id: notif.ID,
            type: notif.type,
            data,
          };
        }).filter(Boolean);

        setNotifications(normalized);
      } else {
        setNotifications([]);
      }

      console.log("Fetched notifications:", result.size);
    } catch (error) {
      console.error("Error fetching notifications:", error);
    }
  };

  // remove from UI immediately
  const removeNotification = (id) => {
    setNotifications((prev) => prev.filter((n) => n.id !== id));
  };

  const createGroup = () => {
    alert('Group Created!');
  };

  const createElection = () => {
    alert('Election Created!');
  };

  return (
    <div className='home'>
      <div className='header'>
        <h1>Home</h1>
        <button onClick={handleRefresh} name='refresh' data-testid="refresh"><Icon icon="material-symbols:refresh-rounded" width="32" height="32"/></button>
      </div>
      <div className='home-container'>
        <div className="home-section">

          <div className='home-card'>
            <h2>Your Notifications</h2>
            {notifications.length === 0 ? (
              <p>No notifications.</p>
            ) : (
              <ul>
                {notifications.slice(0, 2).map(notif => (
                  <li key={notif.id}><Notification isGroupInvite={notif.type} data={notif.data} refresh={fetchNotifications} onRemove={removeNotification} /></li>
                ))}
              </ul>
            )}
            <p className='more-link'><Link to="/notifications">See more...</Link></p>
          </div>

          <div className='home-card'>
            <h2>Your Groups</h2>
            {groups.length === 0 ? (
              <p>No groups found.</p>
            ) : (
              <ul className='groups-list'>
                {groups.slice(0, 2).map(org => (
                  <li key={org.spk}><Group groupName={org.name} groupSpk={org.spk} /></li>
                ))}
              </ul>
            )}
            <p className='more-link'><Link to="/groups">See more...</Link></p>
          </div>


        </div>

        <QuickActions onCreateGroup={handleRefresh} onCreateElection={handleRefresh} />
      </div>
    </div>
  );
};

export default Home;
