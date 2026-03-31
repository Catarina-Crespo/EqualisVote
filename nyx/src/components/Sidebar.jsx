import React from 'react';
import { Link } from 'react-router-dom';
import { Icon } from "@iconify/react";

const Sidebar = () => {
  return (
    <div className="sidebar">
      <ul>
        <li><Link to="/"><Icon icon="material-symbols:home-rounded" width="24" height="24" className='sidebar-icon' />Home</Link></li>
        <li><Link to="/groups"><Icon icon="mdi:account-group" width="24" height="24" className='sidebar-icon' />Groups</Link></li>
        <li><Link to="/elections"><Icon icon="mdi:vote" width="24" height="24" className='sidebar-icon' />Elections</Link></li>
        <li><Link to="/notifications"><Icon icon="mingcute:notification-fill" width="24" height="24" className='sidebar-icon' />Notifications</Link></li>

        <li><Link to="/profile"><Icon icon="heroicons:user-16-solid" width="24" height="24" className='sidebar-icon' />Profile</Link></li>
      </ul>
    </div>
  );
};

export default Sidebar;
