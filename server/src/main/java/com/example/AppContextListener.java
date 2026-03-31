package com.example;

import com.example.db.NewManager;
import jakarta.servlet.ServletContextEvent;
import jakarta.servlet.ServletContextListener;
import jakarta.servlet.annotation.WebListener;


/**
 * Class for anything that might need to be initialized once the server starts
 */
@WebListener
public class AppContextListener implements ServletContextListener {

    @Override
    public void contextInitialized(ServletContextEvent sce) {
        ServerKeyManager.init();
    }

    @Override
    public void contextDestroyed(ServletContextEvent sce) {
        NewManager.getInstance().close();
        ServerKeyManager.destroy();
    }
}
