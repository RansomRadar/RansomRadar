import pandas as pd


def read_irp_file(filepath):
    df = pd.read_csv(filepath, delimiter='\t', on_bad_lines='skip')

    df['is_read'] = (df['major_opr'] == 'IRP_MJ_READ').astype(int)
    df['is_write'] = (df['major_opr'] == 'IRP_MJ_WRITE').astype(int)
    df['is_query_information'] = (df['major_opr'] == 'IRP_MJ_QUERY_INFORMATION').astype(int)

    return df