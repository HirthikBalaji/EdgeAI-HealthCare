"""
VITALIS-Edge: PyTorch Training, BatchNorm Fusion, Post-Training Quantization
Theme: Edge AI & Healthcare - DigiKey electronica India Tech Challenge 2026

Trains the 1D-DSCNN-SE model on multi-modal biosignals, evaluates accuracy,
fuses BatchNorm into Conv layers, quantizes to INT8, and saves weights.
"""

import numpy as np
import os
import sys
import json
import torch
import torch.nn as nn
import torch.optim as optim
from torch.utils.data import TensorDataset, DataLoader

from dataset_generator import generate_full_dataset, CLASS_NAMES
from model_architecture import VitalisDSCNNTorch, fuse_and_quantize_model

def run_training_pipeline(epochs=40, batch_size=32, lr=0.003):
    print("=" * 70)
    print("VITALIS-Edge TinyML Training & Quantization Pipeline")
    print("DigiKey Presents electronica India Tech Challenge 2026")
    print("=" * 70)

    # 1. Generate Dataset
    print("\n[Step 1/5] Generating Multi-Modal Biosignal Dataset...")
    X, y = generate_full_dataset(samples_per_class=400, seed=42)
    total_samples = len(X)
    print(f"Total Dataset Size: {total_samples} windows (250 samples x 5 channels each)")

    # 80/20 Train/Test Split
    split_idx = int(0.8 * total_samples)
    X_train, y_train = X[:split_idx], y[:split_idx]
    X_test, y_test = X[split_idx:], y[split_idx:]

    print(f"  Training Set: {len(X_train)} samples")
    print(f"  Test Set:     {len(X_test)} samples")

    # PyTorch DataLoaders
    train_dataset = TensorDataset(torch.from_numpy(X_train), torch.from_numpy(y_train))
    test_dataset = TensorDataset(torch.from_numpy(X_test), torch.from_numpy(y_test))

    train_loader = DataLoader(train_dataset, batch_size=batch_size, shuffle=True)
    test_loader = DataLoader(test_dataset, batch_size=batch_size, shuffle=False)

    # 2. Initialize Model
    print("\n[Step 2/5] Initializing 1D-DSCNN-SE Model...")
    model = VitalisDSCNNTorch(in_channels=5, num_classes=6)
    param_count = sum(p.numel() for p in model.parameters() if p.requires_grad)
    print(f"  Total Trainable Parameters: {param_count:,}")

    # 3. Training Loop with AdamW and Cosine Annealing
    print("\n[Step 3/5] Starting Model Optimization with AdamW...")
    criterion = nn.CrossEntropyLoss()
    optimizer = optim.AdamW(model.parameters(), lr=lr, weight_decay=1e-4)
    scheduler = optim.lr_scheduler.CosineAnnealingLR(optimizer, T_max=epochs)

    for epoch in range(epochs):
        model.train()
        running_loss = 0.0
        for xb, yb in train_loader:
            optimizer.zero_grad()
            logits = model(xb)
            loss = criterion(logits, yb)
            loss.backward()
            optimizer.step()
            running_loss += loss.item() * xb.size(0)

        scheduler.step()
        epoch_loss = running_loss / len(train_dataset)

        if (epoch + 1) % 5 == 0 or epoch == 0:
            model.eval()
            correct = 0
            with torch.no_grad():
                for xb, yb in test_loader:
                    logits = model(xb)
                    preds = torch.argmax(logits, dim=1)
                    correct += (preds == yb).sum().item()
            test_acc = (correct / len(test_dataset)) * 100.0
            print(f"  Epoch [{epoch+1:2d}/{epochs:2d}] | Loss: {epoch_loss:.4f} | Test Accuracy: {test_acc:.2f}%")

    # 4. Final FP32 Evaluation
    model.eval()
    all_preds_fp32 = []
    all_targets = []
    with torch.no_grad():
        for xb, yb in test_loader:
            logits = model(xb)
            preds = torch.argmax(logits, dim=1)
            all_preds_fp32.extend(preds.cpu().numpy())
            all_targets.extend(yb.cpu().numpy())

    all_preds_fp32 = np.array(all_preds_fp32)
    all_targets = np.array(all_targets)
    fp32_acc = np.mean(all_preds_fp32 == all_targets) * 100.0

    print(f"\n[Step 4/5] Full Floating Point (FP32) Model Accuracy: {fp32_acc:.2f}%")

    # 5. Fused BatchNorm and INT8 Quantization
    print("\n[Step 5/5] Performing BatchNorm Fusion & Symmetric INT8 Quantization...")
    quant_dict = fuse_and_quantize_model(model)

    # Save quantized parameters for C header exporter
    save_path = os.path.join(os.path.dirname(__file__), "trained_quantized_weights.npz")
    np.savez_compressed(save_path, **quant_dict)
    print(f"Saved quantized weights checkpoint to: {save_path}")

    return model, quant_dict

if __name__ == "__main__":
    run_training_pipeline()
