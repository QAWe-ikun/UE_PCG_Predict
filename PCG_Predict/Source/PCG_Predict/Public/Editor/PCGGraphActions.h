#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"

class SGraphPanel;

// 前向声明
class UPCGNode;
class UPCGGraph;

/**
 * PCG Graph 操作工具类
 * 用于创建 PCG 节点和连接
 */
class FPCGGraphActions {
public:
  /**
   * 创建 PCG 节点并连接到目标 Pin
   */
  static bool CreateNodeAndConnect(TSharedPtr<SGraphPanel> GraphPanel,
                                   const FString &NodeTypeName,
                                   UEdGraphPin *TargetPin,
                                   EEdGraphPinDirection Direction,
                                   FVector2D SpawnLocation);

  /**
   * 创建 PCG 节点
   */
  static UPCGNode *CreatePCGNode(UPCGGraph *Graph, const FString &NodeTypeName,
                                 FVector2D Location);

  /**
   * 查找 PCG Settings 类
   */
  static UClass *FindPCGSettingsClass(const FString &NodeTypeName);

  /**
   * 查找/加载类
   */
  static UClass *FindClass(const FString &ClassPath);

  /**
   * 创建 Pin 连接
   */
  static void CreatePinConnection(UEdGraphPin *OutputPin,
                                  UEdGraphPin *InputPin);
};
