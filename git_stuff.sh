#!/bin/bash

FORK_URL="https://github.com/Sahil-M-Deo/Deep-Chess.git"
UPSTREAM_URL="https://github.com/suhas11A/Deep-Chess.git"

echo "========== Deep-Chess Git Helper =========="

if [ ! -d ".git" ]; then
    echo "No git repository found."
    echo "Initializing repository..."

    git init

    git branch -M master
fi

echo
echo "Checking remotes..."

if git remote get-url origin >/dev/null 2>&1; then
    current_origin=$(git remote get-url origin)

    echo "Current origin:"
    echo "$current_origin"

    if [ "$current_origin" != "$FORK_URL" ]; then
        read -p "Replace origin with your fork? (y/n): " ans

        if [ "$ans" = "y" ]; then
            git remote set-url origin "$FORK_URL"
            echo "Origin updated."
        fi
    fi
else
    git remote add origin "$FORK_URL"
    echo "Origin added."
fi

if git remote get-url upstream >/dev/null 2>&1; then
    current_upstream=$(git remote get-url upstream)

    echo "Current upstream:"
    echo "$current_upstream"

    if [ "$current_upstream" != "$UPSTREAM_URL" ]; then
        read -p "Replace upstream with mentor repo? (y/n): " ans

        if [ "$ans" = "y" ]; then
            git remote set-url upstream "$UPSTREAM_URL"
            echo "Upstream updated."
        fi
    fi
else
    git remote add upstream "$UPSTREAM_URL"
    echo "Upstream added."
fi

echo
echo "Checking git identity..."

current_name=$(git config user.name)
current_email=$(git config user.email)

if [ -z "$current_name" ]; then
    read -p "Enter git user.name: " username
    git config user.name "$username"
else
    echo "Using saved user.name: $current_name"
fi

if [ -z "$current_email" ]; then
    read -p "Enter git user.email: " email
    git config user.email "$email"
else
    echo "Using saved user.email: $current_email"
fi

echo
echo "Fetching upstream changes..."

git fetch upstream

echo
read -p "Merge upstream/master into local master? (y/n): " sync_ans

if [ "$sync_ans" = "y" ]; then
    git checkout master
    git merge upstream/master
fi

echo
echo "Git status:"
git status

echo
git add .

if git diff --cached --quiet; then
    echo "Nothing to commit."

    read -p "Push anyway? (y/n): " push_anyway

    if [ "$push_anyway" != "y" ]; then
        exit 0
    fi
else
    echo
    read -p "Commit message: " msg

    git commit -m "$msg"
fi

echo
echo "Pushing to origin/master..."

git push -u origin master

echo
echo "Done."
