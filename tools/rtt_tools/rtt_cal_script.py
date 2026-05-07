# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear
import os
import re
import sys
import pandas as pd
import xlsxwriter
import numpy as np


directory = os.getcwd()

log_files = [f for f in os.listdir(directory) if f.endswith('.txt')]

workbook = xlsxwriter.Workbook(f'output.xlsx', {'nan_inf_to_errors': True})

worksheet_overall = workbook.add_worksheet('Overall')

for log_file in log_files:
    filename = os.path.join(directory, log_file)

    match = re.search(r'(\d+)m', log_file)
    if match:
        n = int(match.group(1)) * 100
    else:
        print(f"Error: could not extract value of n from log file name '{log_file}'")
        continue

    data = []
    row = {}
    with open(filename, "r") as f:
        for line in f:
            for key in ["t1", "t2", "t3", "t4", "fac"]:
                match = re.search(fr"FTM:{key}:(\w+)", line)
                if match:
                    value = int(match.group(1), 16)
                    row[key] = value
            if "FTM:--------------------------------------------" in line:
                if row:
                    if "t1" in row and "t4" in row:
                        row["t4-t1"] = row["t4"] - row["t1"] if row["t4"] > row["t1"] else 4294967296 + row["t4"] - row["t1"]
                    if "t2" in row and "fac" in row:
                        row["t2_correct"] = 128*row["t2"]+3*(row["fac"]-4096)
                    if "t2_correct" in row and "t3" in row:
                        row["t3-t2"]=(128*row["t3"]-row["t2_correct"])*3125/96 if row["t3"] > row["t2"] else  ((262144+row["t3"])*128-row["t2_correct"])*3125/96
                        row["t4_t1-t3_t2"]=row["t4-t1"] - row["t3-t2"]
                        row["rtt"] = abs(row["t4_t1-t3_t2"]-9953353/64)-35085   #base_delay 2g:35085 5g:33013
                        row["distance"]=row["rtt"]*3/200
                        row["error"]= abs(n-row["distance"])
                    data.append(row)
                    row = {}

    if row:
        data.append(row)

    df = pd.DataFrame(data)

    bins = [0] + list(range(10, 10000, 10))
    labels = [f"{x/100:.1f}" for x in bins[1:]]
    df["error_range"] = pd.cut(df["error"], bins=bins, labels=labels, right=False)
    error_distribution = df["error_range"].value_counts().sort_index()
    cumulative_error_distribution = error_distribution.cumsum()
    cumulative_error_percentage = cumulative_error_distribution / len(df) * 100

    worksheet_name = os.path.splitext(log_file)[0]
    worksheet_data = workbook.add_worksheet(worksheet_name)

    for i, col_name in enumerate(df.columns):
        worksheet_data.write(0, i, col_name)

    for i, row in enumerate(df.values):
        worksheet_data.write_row(i + 1, 0, row)

    analyze_df = pd.concat([error_distribution, cumulative_error_distribution, cumulative_error_percentage], axis=1)
    analyze_df.columns = ["count", "cumulative_count", "cumulative_percentage"]
    analyze_df.reset_index(inplace=True)

    worksheet_name = f"{os.path.splitext(log_file)[0]}_{n//100}m_analyze"
    worksheet_analyze = workbook.add_worksheet(worksheet_name)

    for i, col_name in enumerate(analyze_df.columns):
        worksheet_analyze.write(0, i, col_name)

    for i, (index, count, cumulative_count, cumulative_percentage) in enumerate(analyze_df.values):
        int_index = int(float(index) * 100)
        worksheet_analyze.write_number(i + 1, 0, int_index, workbook.add_format({'num_format': '0.00'}))
        worksheet_analyze.write(i + 1, 1, count)
        worksheet_analyze.write(i + 1, 2, cumulative_count)
        worksheet_analyze.write(i + 1, 3, cumulative_percentage / 100,
                                workbook.add_format({'num_format': '0.00%'}))

    chart = workbook.add_chart({'type': 'line'})
    chart.add_series({
        'name':       f'{n//100}m',
        'categories': f'={worksheet_name}!$A$2:$A$31',
        'values':     f'={worksheet_name}!$D$2:$D$31',
    })

    chart.set_x_axis({
        'min': 0,
        'max': 3,
        'major_unit': 0.1,
        'num_format': '0',
    })

    chart.set_y_axis({
        'max': 1.0,
        'major_unit': 0.1,
    })

    chart.set_title({'name': 'Fermion'})

    worksheet_analyze.insert_chart('F2', chart)

overall_chart = workbook.add_chart({'type': 'line'})

for log_file in log_files:
    match = re.search(r'(\d+)m', log_file)
    if match:
        n = int(match.group(1)) * 100
    else:
        continue

    worksheet_name = f"{os.path.splitext(log_file)[0]}_{n//100}m_analyze"
    overall_chart.add_series({
        'name':       f'{n//100}m',
        'categories': f'={worksheet_name}!$A$2:$A$31',
        'values':     f'={worksheet_name}!$D$2:$D$31',
    })

overall_chart.set_x_axis({
    'min': 0,
    'max': 3,
    'major_unit': 0.1,
    'num_format': '0',
})

overall_chart.set_y_axis({
    'max': 1.0,
    'major_unit': 0.1,
})

overall_chart.set_title({'name': 'Overall'})

worksheet_overall.insert_chart('A1', overall_chart)

workbook.close()
