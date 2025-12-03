#pragma once
#include "CoreMinimal.h"
#include "UI/Programs/TrialProgramBase.h"

/** Program A: Memory Defrag Wizard - Tower of Hanoi */
class ELEVATOR_API FTowerOfHanoiProgram : public FTrialProgramBase
{
public:
    FTowerOfHanoiProgram();
protected:
    virtual TSharedRef<SWidget> BuildTrialBody() override;
    virtual void GenerateNewTrial() override;
    virtual void HandlePointerMoved(const FScreenPointerEvent& Event, bool bHandledByPreProcessor) override;
    virtual void HandlePointerPressed(const FScreenPointerEvent& Event, bool bHandledByPreProcessor) override;
    virtual void HandlePointerReleased(const FScreenPointerEvent& Event, bool bHandledByPreProcessor) override;
private:
    void InitializeBlocks();
    bool CanMoveBlock(int32 FromPeg, int32 ToPeg) const;
    void MoveBlock(int32 FromPeg, int32 ToPeg);
    bool CheckWinCondition() const;
    int32 GetPegAtPosition(const FVector2D& Position) const;
    FVector2D GetPegPosition(int32 PegIndex) const;
    FString RenderPegColumn(int32 PegIndex) const;
    
    TArray<TArray<int32>> Pegs; // 3 pegs, each with stack of block sizes
    int32 NumBlocks = 4;
    int32 SelectedPeg = -1;
    int32 HoveredPeg = -1;
    int32 DraggedBlock = -1;  // Size of block being dragged, -1 if none
    int32 DragSourcePeg = -1; // Which peg the dragged block came from
    FVector2D DragPosition;
    int32 MoveCount = 0;
    bool bShowingReindex = false;
    FKey ActivePointerKey = EKeys::Invalid;
};
