import pandas as pd 
from config import RESULT_PATH
from sample_process import sample_process


def main():
    for label in ['benign', 'ransomware']:
        encryption_df = pd.read_csv(f'{RESULT_PATH}\\encryption_detection_result_{label}.csv')
        encryption_df = encryption_df.rename(columns={'predict': 'enc_predict'})
        encryption_df = encryption_df[['Sample', 'Process', 'Second', 'enc_predict']]
        encryption_df['enc_predict'] = encryption_df['enc_predict'].astype(bool)

        tc_df = pd.read_csv(f'{RESULT_PATH}\\tc_detection_result_{label}.csv')
        tc_df = tc_df.rename(columns={'sample': 'Sample', 'process': 'Process', 'predict': 'tc_predict'})
        tc_df = tc_df[['Sample', 'Process', 'Second', 'tc_predict']]
        tc_df['tc_predict'] = tc_df['tc_predict'].astype(bool)
        
        df = pd.merge(encryption_df, tc_df, on=['Sample', 'Process', 'Second'], how='inner')
        
        if label == 'ransomware':
            df = df[df.apply(lambda row: row['Process'] == sample_process.get(row['Sample'], ''), axis=1)]

        df['result'] = df['enc_predict'] & df['tc_predict']
        
        cnt_detected = 0
        total = 0

        for _, sub_df in df.groupby(['Sample', 'Process']):
            total += 1
            if sub_df['result'].any():
                cnt_detected += 1

        if label == 'benign':
            print(f'benign, false positive rate: {round(cnt_detected/total*100, 2)}% ({cnt_detected}/{total})')
        elif label == 'ransomware':
            print(f'ransomware: recall: {round(cnt_detected/total*100, 2)}% ({cnt_detected}/{total})')


if __name__ == '__main__':
    main()