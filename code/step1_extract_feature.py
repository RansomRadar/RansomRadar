from config import HPC_ROOT_path, IRP_ROOT_PATH, RAW_ROOT_PATH, FEATURE_PATH
from read_hpc_file import read_hpc_file
from read_irp_file import read_irp_file

import os
import pandas as pd
import numpy as np
import pandas as pd


# calculate features for every second
def calculate_1s_feature(filepath, targetpath):  
    print(f'extract 1s feature for {targetpath}')

    if os.path.exists(targetpath):
        return

    df = read_hpc_file(filepath)

    result_df = pd.DataFrame(columns=[
        'Sample',
        'Process',
        'Second',
        'avg_branchinstructionrate',
        'std_branchinstructionrate',
        'avg_branchmispredictsrate',
        'std_branchmispredictsrate',
        'avg_llcrefrate',
        'std_llcrefrate',
        'avg_llcmissrate',
        'std_llcmissrate',
    ])

    counter_one_hot = pd.get_dummies(df['Counter'])
    df = pd.concat([df, counter_one_hot], axis=1)
    df['Timestamp'] = df['Timestamp'].astype(np.float64)
    df['Second'] = (df['Timestamp'] / 10000000).astype(np.int64)
    df['100ms'] = (df['Timestamp'] / 1000000).astype(np.int64)
    sample = df['Sample'].iloc[0]

    for process, process_df in df.groupby('Process'):
        for second, second_df in process_df.groupby('Second'):
            branchinstructionrate_lst = []
            branchmispredictsrate_lst = []
            llcrefrate_lst = []
            llcmissrate_lst = []
            instruction_lst = []
            branchinstruction_lst = []
            llcref_lst = []
            cnt = 0

            for _, sub_df in second_df.groupby('100ms'):
                try:
                    instructions = sub_df['InstructionsRetiredFixed'].sum()
                    branchinstructions = sub_df['BranchInstructionRetired'].sum()
                    branchmispredicts = sub_df['BranchMispredictsRetired'].sum()
                    llcrefs = sub_df['LLCReference'].sum()
                    llcmisses = sub_df['LLCMisses'].sum()
                    if instructions == 0 or branchinstructions == 0 or llcrefs == 0:
                        continue
                    branchinstructionrate_lst.append(branchinstructions / instructions)
                    branchmispredictsrate_lst.append(branchmispredicts / branchinstructions)
                    llcrefrate_lst.append(llcrefs / instructions)
                    llcmissrate_lst.append(llcmisses / llcrefs)
                    instruction_lst.append(instructions)
                    branchinstruction_lst.append(branchinstructions)
                    llcref_lst.append(llcrefs)
                    cnt += 1
                except:
                    pass

            if cnt == 0:
                continue

            result_df.loc[len(result_df)] = {
                'Sample': sample,
                'Process': process,
                'Second': second,
                'avg_branchinstructionrate': np.mean(branchinstructionrate_lst),
                'std_branchinstructionrate': np.std(branchinstructionrate_lst),
                'avg_branchmispredictsrate': np.mean(branchmispredictsrate_lst),
                'std_branchmispredictsrate': np.std(branchmispredictsrate_lst),
                'avg_llcrefrate': np.mean(llcrefrate_lst),
                'std_llcrefrate': np.std(llcrefrate_lst),
                'avg_llcmissrate': np.mean(llcmissrate_lst),
                'std_llcmissrate': np.std(llcmissrate_lst),
            } 

    result_df.to_csv(targetpath)


# calculate features for every 100ms
def calculate_100ms_feature(filepath, targetpath):
    print(f'calculate 100ms feature for {targetpath}')

    if os.path.exists(targetpath):
        return
    
    df = read_hpc_file(filepath)

    result_df = pd.DataFrame(columns=[
        'Sample',
        'Process',
        'fromtime',
        'totime',
        'instructions',
        'branchinstructions',
        'branchmispredicts',
        'llcreferences',
        'llcmisses'
    ])

    counter_one_hot = pd.get_dummies(df['Counter'])
    df = pd.concat([df, counter_one_hot], axis=1)
    df['100ms'] = (df['Timestamp'] / 1000000).astype(np.int64)
    sample = df['Sample'].iloc[0]

    for process, process_df in df.groupby('Process'):
        for ms100, ms100_df in process_df.groupby('100ms'):
            result_df.loc[len(result_df)] = {
                'Sample': sample,
                'Process': process,
                'fromtime': ms100 * 1000000,
                'totime': (ms100+1) * 1000000,
                'instructions': ms100_df['InstructionsRetiredFixed'].sum(),
                'branchinstructions': ms100_df['BranchInstructionRetired'].sum(),
                'branchmispredicts': ms100_df['BranchMispredictsRetired'].sum(),
                'llcreferences': ms100_df['LLCReference'].sum(),
                'llcmisses': ms100_df['LLCMisses'].sum()
            }

    result_df.to_csv(targetpath)


# calculate feature for lstm
def calculate_lstm_feature(irp_path, hpc_path, output_path, label):  
    print(f'calculate lstm feature for {output_path}')

    if os.path.exists(output_path):
        return
      
    feature_cols = ['sample', 'process', 'starttime', 'label']
    for i in range(10):
        feature_cols.append(f'read_{i}')
        feature_cols.append(f'write_{i}')
        feature_cols.append(f'rename_{i}')
        feature_cols.append(f'delete_{i}')
        feature_cols.append(f'query_information_{i}')
        feature_cols.append(f'filesize_{i}')
        feature_cols.append(f'instructions_{i}')
        feature_cols.append(f'branchinstructions_{i}')
        feature_cols.append(f'branchmispredicts_{i}')
        feature_cols.append(f'llcrefs_{i}')
        feature_cols.append(f'llcmisses_{i}')

    feature_df = pd.DataFrame(columns=feature_cols)
    
    # get sample name
    sample = os.path.basename(irp_path).split('.')[0]
    
    try:
        irp_df = read_irp_file(irp_path)
        hpc_df = pd.read_csv(hpc_path, index_col=0) 

        hpc_df['Second'] = hpc_df['fromtime'].apply(lambda x: int(x / 1e7))
        irp_df['file_basename'] = irp_df['file_name'].apply(lambda x: x.split('.')[0].lower())

        # consider every process separately
        for (sample, process), sub_df in hpc_df.groupby(['Sample', 'Process']):
            SecondList = list(sub_df['Second'].unique())
            
            # accessed files
            read_files = set()

            for second in SecondList:
                starttime = second * 10000000
                row_data = {}
                row_data['sample'] = sample
                row_data['process'] = process
                row_data['starttime'] = starttime
                row_data['label'] = 0 if label == 'benign' else 1
                for i in range(10):
                    ms_start = starttime + i * 1000000
                    ms_end = starttime + (i + 1) * 1000000
                    hpc_data = hpc_df[hpc_df['fromtime'] == ms_start]
                    irp_data = irp_df[(ms_start <= irp_df['time']) & (irp_df['time'] < ms_end)]

                    # files being read before
                    read_files.update(set(irp_data[irp_data['is_write']==0]['file_basename'].unique()))

                    row_data[f'read_{i}'] = irp_data['is_read'].sum()
                    row_data[f'write_{i}'] = irp_data['is_write'].sum()
                    row_data[f'rename_{i}'] = irp_data['is_rename'].sum()
                    row_data[f'delete_{i}'] = irp_data['is_delete'].sum()
                    row_data[f'query_information_{i}'] = irp_data['is_query_information'].sum()
                    row_data[f'filesize_{i}'] = irp_data[irp_data['file_basename'].isin(read_files)]['file_size'].sum()
                    row_data[f'instructions_{i}'] = hpc_data['instructions'].sum()
                    row_data[f'branchinstructions_{i}'] = hpc_data['branchinstructions'].sum()
                    row_data[f'branchmispredicts_{i}'] = hpc_data['branchmispredicts'].sum()
                    row_data[f'llcrefs_{i}'] = hpc_data['llcreferences'].sum()
                    row_data[f'llcmisses_{i}'] = hpc_data['llcmisses'].sum()

                feature_df.loc[len(feature_df)] = row_data

        # save result
        feature_df.to_csv(output_path)
    except Exception as e:
        print(f'error while processing {irp_path} {hpc_path} {e}')
        return


def main():
    for label in ['benign', 'ransomware']:
        for file in os.listdir(f'{RAW_ROOT_PATH}\\{label}'):
            sample = file.split('.')[0]

            calculate_1s_feature(f'{HPC_ROOT_path}\\{label}\\{sample}.csv', f'{FEATURE_PATH}\\1s\\{label}\\{sample}.csv')

            calculate_100ms_feature(f'{HPC_ROOT_path}\\{label}\\{sample}.csv', f'{FEATURE_PATH}\\100ms\\{label}\\{sample}.csv')
            
            calculate_lstm_feature(f'{IRP_ROOT_PATH}\\{label}\\{sample}.txt', f'{FEATURE_PATH}\\100ms\\{label}\\{sample}.csv', f'{FEATURE_PATH}\\lstm\\{label}\\{sample}.csv', label)


if __name__ == '__main__':
    main()
