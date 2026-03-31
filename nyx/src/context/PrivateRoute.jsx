// src/components/RequireKeys.jsx
import React from "react";
import { Navigate, useLocation } from "react-router-dom";
import { useUser } from "../context/UserContext";

const PrivateRoute = ({ element: Component, ...rest }) => {
  const { keys } = useUser();
  const location = useLocation();

  if (!keys) {
    // Redirect to profile, but remember the route the user tried
    return <Navigate to="/profile" state={{ from: location }} replace />;
  }

  return (
    <Route
      {...rest}
      render={(props) => {
        if (loading) {
          return <div>Loading...</div>; // TODO: Replace with a spinner
        }
        // If authenticated and access level is valid, render the component
        return <Component {...props} />;
      }}
    />
  );
};

export default PrivateRoute;
