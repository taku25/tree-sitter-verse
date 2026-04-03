using { /Verse.org/Verse }

# An example class
player_character<public> := class:
    Name<public>:string
    var Level:int = 1
    var Score:int = 0

    GainScore(Amount:int)<transacts>:void =
        set Score += Amount

    GetLevel()<reads>:int = Level

item_rarity := enum{
    common
    uncommon
    rare
    epic
    legendary
}

game_item := struct{
    Name:string
    Damage:float = 0.0
    Weight:float = 1.0
}

RunExample<public>()<suspends>:void =
    Hero := player_character{Name := "Verse Hero"}
    Hero.GainScore(100)
    if (Hero.Score > 50):
        X := "High score!"
    else:
        X := "Keep playing!"
