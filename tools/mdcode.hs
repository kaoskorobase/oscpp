import Text.Pandoc

extractCodeBlock :: Block -> [String]
extractCodeBlock (CodeBlock (id, classes, namevals) contents) | "cpp" `elem` classes = [contents]
extractCodeBlock _ = []

extractCode :: Pandoc -> [String]
extractCode = queryWith extractCodeBlock

readDoc :: String -> Pandoc
readDoc = readMarkdown def

main :: IO ()
main = interact (unlines . extractCode . readDoc)
