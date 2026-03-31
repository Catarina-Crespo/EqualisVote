import json
from statistics import mean, stdev

def nice(params):
    n, t, D = tuple(map(int, params.split("_")))
    return {
        "n": f"$n = {n}$",
        "t": f"$t = {t}$",
        "D": f"$\\abs{{\\votedomain}} = {D}$"
    }

with open("benchmarks.json") as f:
    benchmarks = json.load(f)

table = ""

for params in benchmarks:
    runtime = {}
    runtime["ve_prove"] = [benchmarks[params][i]["runtime"]["ve_prove"] for i in range(len(benchmarks[params]))]
    runtime["ve_verify"] = [benchmarks[params][i]["runtime"]["ve_verify"] for i in range(len(benchmarks[params]))]
    runtime["ve_all"] = [benchmarks[params][i]["runtime"]["ve_prove"] + benchmarks[params][i]["runtime"]["ve_verify"] for i in range(len(benchmarks[params]))]
    runtime["user_vep_prove"] = [benchmarks[params][i]["runtime"]["user_vep_prove"] for i in range(len(benchmarks[params]))]
    runtime["user_vep_verify"] = [benchmarks[params][i]["runtime"]["user_vep_verify"] for i in range(len(benchmarks[params]))]
    runtime["user_vep_all"] = [benchmarks[params][i]["runtime"]["user_vep_prove"] + benchmarks[params][i]["runtime"]["user_vep_verify"] for i in range(len(benchmarks[params]))]
    runtime["server_vep_prove"] = [benchmarks[params][i]["runtime"]["server_vep_prove"] for i in range(len(benchmarks[params]))]
    runtime["server_vep_verify"] = [benchmarks[params][i]["runtime"]["server_vep_verify"] for i in range(len(benchmarks[params]))]
    runtime["server_vep_all"] = [benchmarks[params][i]["runtime"]["server_vep_prove"] + benchmarks[params][i]["runtime"]["server_vep_verify"] for i in range(len(benchmarks[params]))]
    runtime["intersection"] = [benchmarks[params][i]["runtime"]["intersection"] for i in range(len(benchmarks[params]))]
    runtime["full_protocol"] = [benchmarks[params][i]["runtime"]["full_protocol"] for i in range(len(benchmarks[params]))]

    bandwidth = {}
    bandwidth["ve_statement"] = [benchmarks[params][i]["bandwidth"]["ve_statement"] for i in range(len(benchmarks[params]))]
    bandwidth["ve_proof"] = [benchmarks[params][i]["bandwidth"]["ve_proof"] for i in range(len(benchmarks[params]))]
    bandwidth["user_vep_input"] = [benchmarks[params][i]["bandwidth"]["user_vep_input"] for i in range(len(benchmarks[params]))]
    bandwidth["user_vep_statement"] = [benchmarks[params][i]["bandwidth"]["user_vep_statement"] for i in range(len(benchmarks[params]))]
    bandwidth["user_vep_proof"] = [benchmarks[params][i]["bandwidth"]["user_vep_proof"] for i in range(len(benchmarks[params]))]
    bandwidth["server_vep_input"] = [benchmarks[params][i]["bandwidth"]["server_vep_input"] for i in range(len(benchmarks[params]))]
    bandwidth["server_vep_statement"] = [benchmarks[params][i]["bandwidth"]["server_vep_statement"] for i in range(len(benchmarks[params]))]
    bandwidth["server_vep_proof"] = [benchmarks[params][i]["bandwidth"]["server_vep_proof"] for i in range(len(benchmarks[params]))]
    bandwidth["ballots"] = [benchmarks[params][i]["bandwidth"]["ballots"] for i in range(len(benchmarks[params]))]
    bandwidth["full_protocol"] = [benchmarks[params][i]["bandwidth"]["full_protocol"] for i in range(len(benchmarks[params]))]

    table += "\n\t\t   & total & %.1f & %.1f & %.1f \\\\"  % (                                                          mean(runtime["full_protocol"])/1000,     stdev(runtime["full_protocol"])/1000,     (bandwidth["full_protocol"][0])/1024)
    table += "\n\t\t%s & \\ve.\\Evaloninput \\& check            & %.1f & %.1f & %.1f \\\\"        % (   nice(params)["n"] ,     mean(runtime["ve_all"])/1000,          stdev(runtime["ve_all"])/1000,          (bandwidth["ve_statement"][0] + bandwidth["ve_proof"][0])/1024)
    # table += "\n\t\t%s & \\ve.\\Evaloninput             & %.1f & %.1f & %.1f \\\\"        % (   nice(params)["n"] ,     mean(runtime["ve_prove"])/1000,          stdev(runtime["ve_prove"])/1000,          (bandwidth["ve_statement"][0] + bandwidth["ve_proof"][0])/1024)
    # table += "\n\t\t%s & \\ve check             & %.1f & %.1f & --- \\\\"         % (           nice(params)["t"] ,     mean(runtime["ve_verify"])/1000,         stdev(runtime["ve_verify"])/1000)
    table += "\n\t\t%s & \\vep.\\Evaloninput \\& check (\\user)      & %.1f & %.1f & %.1f \\\\"      % (  nice(params)["t"] ,     mean(runtime["user_vep_all"])/1000,    stdev(runtime["user_vep_all"])/1000,    (bandwidth["user_vep_input"][0] + bandwidth["user_vep_statement"][0] + bandwidth["user_vep_proof"][0])/1024)
    # table += "\n\t\t%s & \\vep.\\Evaloninput (\\user)      & %.1f & %.1f & %.1f \\\\"      % (  nice(params)["D"] ,     mean(runtime["user_vep_prove"])/1000,    stdev(runtime["user_vep_prove"])/1000,    (bandwidth["user_vep_input"][0] + bandwidth["user_vep_statement"][0] + bandwidth["user_vep_proof"][0])/1024)
    # table += "\n\t\t   & \\vep check (\\user)      & %.1f & %.1f & --- \\\\"         % (                                mean(runtime["user_vep_verify"])/1000,   stdev(runtime["user_vep_verify"])/1000)
    table += "\n\t\t%s   & \\vep.\\Evaloninput \\& check (\\srv)       & %.1f & %.1f & %.1f \\\\"        % (  nice(params)["D"] ,                     mean(runtime["server_vep_all"])/1000,  stdev(runtime["server_vep_all"])/1000,  (bandwidth["server_vep_input"][0] + bandwidth["server_vep_statement"][0] + bandwidth["server_vep_proof"][0])/1024)
    # table += "\n\t\t   & \\vep.\\Evaloninput (\\srv)       & %.1f & %.1f & %.1f \\\\"        % (                        mean(runtime["server_vep_prove"])/1000,  stdev(runtime["server_vep_prove"])/1000,  (bandwidth["server_vep_input"][0] + bandwidth["server_vep_statement"][0] + bandwidth["server_vep_proof"][0])/1024)
    # table += "\n\t\t   & \\vep check (\\srv)       & %.1f & %.1f & --- \\\\"         % (                                mean(runtime["server_vep_verify"])/1000, stdev(runtime["server_vep_verify"])/1000)
    table += "\n\t\t   & ballot intersection   & %.1f & %.1f & %.1f \\\\\\midrule" % (                                  mean(runtime["intersection"])/1000,      stdev(runtime["intersection"])/1000,      (bandwidth["ballots"][0])/1024)

print(table)
