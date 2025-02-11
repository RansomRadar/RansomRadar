from config import RESULT_PATH, FEATURE_PATH, MODEL_PATH
import os
import pandas as pd
from joblib import load


def merge_dfs(dir):
    files = [f for f in os.listdir(dir)]
    all_dfs = [pd.read_csv(os.path.join(dir, f), index_col=0) for f in files]
    merged_df = pd.concat(all_dfs, ignore_index=True)
    merged_df.reset_index(drop=True, inplace=True)
    return merged_df


features = [
    'avg_branchinstructionrate',
    'std_branchinstructionrate',
    'avg_branchmispredictsrate',
    'std_branchmispredictsrate',
    'avg_llcrefrate',
    'std_llcrefrate',
    'avg_llcmissrate',
    'std_llcmissrate',
]


def main():
    scaler = load(f'{MODEL_PATH}\\encryption_detection_scaler.joblib')
    clf = load(f'{MODEL_PATH}\\encryption_detection_clf.joblib')
    
    for label in ['benign', 'ransomware']:
        feature_df = merge_dfs(f'{FEATURE_PATH}\\1s\\{label}')
        feature_df['predict'] = clf.predict(scaler.transform(feature_df[features].values))
        feature_df.to_csv(f'{RESULT_PATH}\\encryption_detection_result_{label}.csv')

if __name__ == '__main__':
    main()