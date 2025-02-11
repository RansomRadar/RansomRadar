import pandas as pd
import os


def read_hpc_file(filepath):
    df = pd.read_csv(filepath, on_bad_lines='skip', low_memory=False)
    df = df.dropna()

    sample = os.path.basename(filepath).split('.')[0]
    dir = os.path.dirname(filepath)

    if not os.path.exists(f'{dir}/{sample}_starttime.txt'):
        return None
    
    with open(f'{dir}/{sample}_starttime.txt', 'r') as f:
        starttime = int(f.read())

    df.rename(columns={'Timestamp (ms)': 'Timestamp'}, inplace=True)

    # transform timestamp
    df['Timestamp'] = df['Timestamp'].str.split('.').str[0]
    df['Timestamp'] = df['Timestamp'].str.replace(',', '', regex=False).astype(int) * 10000 + starttime

    # add one colume to record sample name
    df['Sample'] = sample
    
    # select specific columns
    df = df[['Sample', 'Counter', 'Process', 'Timestamp']]

    return df
