import json
import os

for folder in ["Traces/Counters", "Traces/NoCounters"]:
    for filename in os.listdir(folder):
        file_path = os.path.join(folder, filename)
        with open(file_path, "r") as f:
            data = json.load(f)
        for trace in data['data']:
            traceId = trace['traceID']
            for span in trace['spans']:
                if span['references'] and span['references'][0]['refType'] == 'CHILD_OF' and span['references'][0]['traceID'] == traceId:
                    type_dir = folder.split("/")[-1]
                    output_dir = os.path.join("Output", type_dir)
                    os.makedirs(output_dir, exist_ok=True)
                    output_path = os.path.join(output_dir, filename.split(".")[0] + "_latencies.txt")

                    with open(output_path, "a") as f_out:
                        f_out.write(str(span['references'][0]['span']['duration']) + "\n")
                    break


