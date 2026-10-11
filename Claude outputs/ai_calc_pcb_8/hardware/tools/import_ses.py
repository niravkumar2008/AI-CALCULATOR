import pcbnew, sys
b = pcbnew.LoadBoard(sys.argv[1])
ok = pcbnew.ImportSpecctraSES(b, sys.argv[2])
print('ses', ok, 'tracks', len(b.GetTracks()))
b.Save(sys.argv[3] if len(sys.argv) > 3 else sys.argv[1])
