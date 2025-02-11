import pandas as pd
from read_hpc_file import read_hpc_file
from read_irp_file import read_irp_file
from sklearn.preprocessing import MinMaxScaler, StandardScaler
import os
from config import *
import torch
import torch.nn as nn
import torch.optim as optim
from torch.utils.data import Dataset, DataLoader
import pandas as pd
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import StandardScaler
from sklearn.metrics import accuracy_score, precision_score, recall_score, f1_score
import torch
import torch.nn as nn
import torch.nn.functional as F
import torch.optim as optim


import pandas as pd
import os
from torch.utils.data import Dataset
from joblib import load
import torch
from config import FEATURE_PATH, RESULT_PATH, MODEL_PATH


features = []
for i in range(10):
    features.append(f'read_{i}')
    features.append(f'write_{i}')
    features.append(f'rename_{i}')
    features.append(f'delete_{i}')
    features.append(f'filesize_{i}')
    features.append(f'instructions_{i}')
    features.append(f'branchinstructions_{i}')
    features.append(f'branchmispredicts_{i}')
    features.append(f'llcrefs_{i}')
    features.append(f'llcmisses_{i}')


def merge_dfs(dir):
    all_dfs = [pd.read_csv(os.path.join(dir, f), index_col=0) for f in os.listdir(dir)]
    merged_df = pd.concat(all_dfs, ignore_index=True)
    merged_df.reset_index(drop=True, inplace=True)
    return merged_df


class MyDataset(Dataset):
    def __init__(self, data, labels):
        self.data = data
        self.labels = labels

    def __len__(self):
        return len(self.data)

    def __getitem__(self, idx):
        return self.data[idx], self.labels[idx]


class LSTMModel(nn.Module):
    def __init__(self, input_size, hidden_size, num_layers, num_classes):
        super(LSTMModel, self).__init__()
        self.lstm = nn.LSTM(input_size, hidden_size, num_layers, batch_first=True)
        self.fc = nn.Linear(hidden_size, num_classes)

    def forward(self, x):
        out, _ = self.lstm(x)
        out = out[:, -1, :]
        out = self.fc(out)
        return out


def main():
    scaler = load(f'{MODEL_PATH}\\tc_detection_scaler.joblib')

    clf = LSTMModel(input_size=11, hidden_size=50, num_layers=1, num_classes=2)
    clf.load_state_dict(torch.load(f'{MODEL_PATH}\\tc_detection_clf.pth'))

    for label in ['benign', 'ransomware']:
        feature_df = merge_dfs(f'{FEATURE_PATH}\\lstm\\{label}')

        X = feature_df[features].values
        # normalized
        X = scaler.transform(X)
        # transform to time series
        X = X.reshape(-1, 10, X.shape[1] // 10)
        # transform to tensor
        X = torch.tensor(X, dtype=torch.float32)

        _, pred = torch.max(clf(X), 1)
        pred = pred.view(-1).tolist()
        feature_df['predict'] = pred

        feature_df['Second'] = feature_df['starttime'].apply(lambda x: x // 10000000)

        feature_df.to_csv(f'{RESULT_PATH}\\tc_detection_result_{label}.csv')


if __name__ == '__main__':
    main()
