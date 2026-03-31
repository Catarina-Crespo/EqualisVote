# EqualisVote

```
├── benchmark/                      # Files to retrieve metrics and compute plots
├── nyx (client)/
│   ├── binaries/                   # .js and .wasm files compiled from the c++ protocol with emscripten (for Web/Node.js)
│   ├── src/                        # Javascript, CSS and HTML
│   │   └── backend/wasmAPI.js      # API to call the WASM binaries or send requests to the middleware
│   ├── app.js                      # Node.js middleware (performs the requests to the servers)
│   ├── simulateClients.js          # Simulation with 5 clients
│   ├── simulateElection.js         # Simulation with a variable number of group members to test the election's flow
│   ├── simulateME.js               # Same as simulateElection, but with metrics
│   ├── simulateNClients.js         # Simulation with a variable number of clients to test the user's voting flow 
│   └── simulateMU.js               # Same as simulateNClients, but with metrics
│
├── protocol - COLBAC/
│   └── src/ 
│       ├── modules/                # Folder with the protocol modules (create_user, create_group, ...)
│       └── protocol_run.cpp        # File with the original protocol
│
├── elections-server/               # Third-party Server
│   ├── binaries/                   # .js and .wasm files compiled from the c++ protocol with emscripten (for Web/Node.js)
│   └── server.js                   # File with the code for the Third-party server
│
├── server/                         # Java Server (Main Server)
│   └── src/
│       └── main/
│           ├── binaries/           # C++ binaries generated from the protocol (.dll libraries)
│           ├── java/com.example/
│           │   ├── db/             # Database handlers
│           │   ├── websockets/     # WebSocket handlers
│           │   ├── servlets/       # Java classes with the servlet code (actual server logic)
│           │   ├── AppContext.java # File to initialize keys and DB managers
│           │   ├── Deadline.java   # Deadline checker
│           │   └── ...             # Other useful classes
│           │                       
│           ├── test/               # JUnit tests for the servlets
│           └── webapp/             # Required files for Tomcat deployment
└── README.md
```

## Main Server

The Tomcat server is available at the `/server` folder. You can run it and access it at `http://localhost:8080/server_war`, and a "Hello World!" message should be displayed on the browser if you try to access the URL above

## Third-party

The Third-party Server is available in the `/elections-server` folder. To run it, type `npm run start` in the terminal. It'll be accessible at `http://localhost:5000`.

## Client App

The `/nyx` (client) folder contains a prototype of a client app. To run it, type `node app.js` in a terminal opened in the `/nyx` folder, or to use `nodemon`, type `npm run start`.

You should see a message on the terminal stating that `Client app running at http://localhost:3000`. If you visit the browser on that URL, a page to test sending the requests to the Tomcat server should be displayed. 

To run the simulations on the client folder:
- `simulateClients.js` - run with `npm run simulate` to simulate a flow with 5 clients
- `simulateNClients.js` - run with `npm run simulateN -- <initial_group_size> <n_voting_users>` or just `npm run simulateN` to simulate a user-voting flow for multiple users
- `simulateElection.js` - run with `npm run simulateE` to simulate an election flow

To run the simulation **first run app.js in the terminal** (`npm run start`), since the simulations will use the middleware provided by it and then open another terminal to run the simulation itself

The interface of the client app is available by running `npm run dev` and going to `http://localhost:5173/`


## Benchmark

To obtain the results:

1. Run the Tomcat Server
2. Run the Elections' Server (`npm run start` in the terminal opened in `/elections-server`)
3. Run the Client Server (`npm run server` in the terminal opened in `/nyx`)

With everything running, go to the `/benchmark` folder and:

- to run the User Join Benchmark: `node benchmark_user.js` (the results will be written to `user_benchmark.csv`)
- to run the Election Benchmark: `node benchmark_elections.js` (the results will be written to `election_benchmark.csv`)
