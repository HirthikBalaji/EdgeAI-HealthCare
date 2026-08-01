"""
VITALIS-Edge: Optimized 1D-DSCNN-SE Architecture for Biosignal Classification
Theme: Edge AI & Healthcare - DigiKey electronica India Tech Challenge 2026

Architecture:
  - Input: (B, 5 channels, 250 samples)
  - Conv1D (5 -> 32, kernel=15, stride=2, padding=7) + BatchNorm + ReLU (L: 125)
  - MaxPool1D (kernel=2, stride=2) (L: 62)
  - DepthwiseConv1D (32 -> 32, kernel=5, stride=1, padding=2, groups=32) + BatchNorm + ReLU (L: 62)
  - PointwiseConv1D (32 -> 32, kernel=1, stride=1) + BatchNorm + ReLU (L: 62)
  - Squeeze-and-Excitation (32 -> 8 -> 32)
  - Global Average Pooling (32)
  - Linear (32 -> 6)
"""

import torch
import torch.nn as nn
import numpy as np

class SqueezeExcitation1D(nn.Module):
    def __init__(self, channels=32, reduction=4):
        super().__init__()
        self.fc1 = nn.Linear(channels, channels // reduction, bias=True)
        self.relu = nn.ReLU()
        self.fc2 = nn.Linear(channels // reduction, channels, bias=True)
        self.sigmoid = nn.Sigmoid()

    def forward(self, x):
        # x: (B, C, L)
        b, c, _ = x.size()
        gap = x.mean(dim=2) # (B, C)
        z1 = self.relu(self.fc1(gap))
        z2 = self.sigmoid(self.fc2(z1)).view(b, c, 1)
        return x * z2

class VitalisDSCNNTorch(nn.Module):
    def __init__(self, in_channels=5, num_classes=6):
        super().__init__()
        # Conv1D Layer 1 (5 -> 32, k=15, s=2)
        self.conv1 = nn.Conv1d(in_channels, 32, kernel_size=15, stride=2, padding=7, bias=False)
        self.bn1 = nn.BatchNorm1d(32)
        self.relu1 = nn.ReLU()
        self.pool1 = nn.MaxPool1d(kernel_size=2, stride=2) # 125 -> 62

        # Depthwise Conv1D (32 -> 32, k=5, s=1, groups=32)
        self.dwconv = nn.Conv1d(32, 32, kernel_size=5, stride=1, padding=2, groups=32, bias=False)
        self.bn2 = nn.BatchNorm1d(32)
        self.relu2 = nn.ReLU()

        # Pointwise Conv1D (32 -> 32, k=1, s=1)
        self.pwconv = nn.Conv1d(32, 32, kernel_size=1, stride=1, padding=0, bias=False)
        self.bn3 = nn.BatchNorm1d(32)
        self.relu3 = nn.ReLU()

        # Squeeze-and-Excitation
        self.se = SqueezeExcitation1D(channels=32, reduction=4)

        # Global Average Pooling + Classifier
        self.fc = nn.Linear(32, num_classes, bias=True)

    def forward(self, x):
        # x: (B, 250, 5) -> permute to (B, 5, 250)
        if x.ndim == 3 and x.shape[1] == 250:
            x = x.permute(0, 2, 1)

        x = self.relu1(self.bn1(self.conv1(x)))
        x = self.pool1(x)
        x = self.relu2(self.bn2(self.dwconv(x)))
        x = self.relu3(self.bn3(self.pwconv(x)))
        x = self.se(x)
        x = x.mean(dim=2) # GAP (B, 32)
        logits = self.fc(x)
        return logits

def fuse_and_quantize_model(model):
    """
    Fuses BatchNorm layers into Conv weights and biases, then quantizes to INT8.
    """
    model.eval()
    quant_dict = {}

    def fuse_conv_bn(conv, bn):
        w = conv.weight.detach().cpu().numpy()
        gamma = bn.weight.detach().cpu().numpy()
        beta = bn.bias.detach().cpu().numpy()
        mean = bn.running_mean.detach().cpu().numpy()
        var = bn.running_var.detach().cpu().numpy()
        eps = bn.eps

        inv_std = 1.0 / np.sqrt(var + eps)
        # Reshape for broadcasting
        if w.ndim == 3: # (C_out, C_in, K)
            scale = (gamma * inv_std)[:, None, None]
            fused_w = w * scale
            fused_b = beta - mean * gamma * inv_std
        return fused_w, fused_b

    w_c1, b_c1 = fuse_conv_bn(model.conv1, model.bn1)
    w_dw, b_dw = fuse_conv_bn(model.dwconv, model.bn2)
    w_pw, b_pw = fuse_conv_bn(model.pwconv, model.bn3)

    w_se1 = model.se.fc1.weight.detach().cpu().numpy()
    b_se1 = model.se.fc1.bias.detach().cpu().numpy()
    w_se2 = model.se.fc2.weight.detach().cpu().numpy()
    b_se2 = model.se.fc2.bias.detach().cpu().numpy()
    w_fc = model.fc.weight.detach().cpu().numpy()
    b_fc = model.fc.bias.detach().cpu().numpy()

    tensors = [
        ('W_c1', w_c1), ('b_c1', b_c1),
        ('W_dw', w_dw), ('b_dw', b_dw),
        ('W_pw', w_pw), ('b_pw', b_pw),
        ('W_se1', w_se1), ('b_se1', b_se1),
        ('W_se2', w_se2), ('b_se2', b_se2),
        ('W_fc', w_fc), ('b_fc', b_fc)
    ]

    for name, arr in tensors:
        max_val = np.max(np.abs(arr))
        scale = float(max_val / 127.0) if max_val > 0 else 1.0
        q_int8 = np.clip(np.round(arr / scale), -128, 127).astype(np.int8)
        quant_dict[name] = q_int8
        quant_dict[f'scale_{name}'] = scale

    return quant_dict
