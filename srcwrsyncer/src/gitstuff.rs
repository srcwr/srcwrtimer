// SPDX-License-Identifier: CC0-1.0
// Copyright libgit2 contributors

// from https://github.com/rust-lang/git2-rs/blob/61f8afdd52e0168145dcc21b18d8824467d4d1c7/examples/pull.rs
// with unused stuff deleted (println!s, non-ff merge)

/*
 * libgit2 "pull" example - shows how to pull remote data into a local branch.
 *
 * Written by the libgit2 contributors
 *
 * To the extent possible under law, the author(s) have dedicated all copyright
 * and related and neighboring rights to this software to the public domain
 * worldwide. This software is distributed without any warranty.
 *
 * You should have received a copy of the CC0 Public Domain Dedication along
 * with this software. If not, see
 * <http://creativecommons.org/publicdomain/zero/1.0/>.
 */

pub fn do_fetch<'a>(
	repo: &'a git2::Repository,
	refs: &[&str],
	remote: &'a mut git2::Remote,
) -> Result<git2::AnnotatedCommit<'a>, git2::Error> {
	let mut fo = git2::FetchOptions::new();
	// Always fetch all tags.
	// Perform a download and also update tips
	fo.download_tags(git2::AutotagOption::All);
	remote.fetch(refs, Some(&mut fo), None)?;

	let fetch_head = repo.find_reference("FETCH_HEAD")?;
	Ok(repo.reference_to_annotated_commit(&fetch_head)?)
}

fn fast_forward(
	repo: &git2::Repository,
	lb: &mut git2::Reference,
	rc: &git2::AnnotatedCommit,
) -> Result<(), git2::Error> {
	let name = match lb.name() {
		Some(s) => s.to_string(),
		None => String::from_utf8_lossy(lb.name_bytes()).to_string(),
	};
	let msg = format!("Fast-Forward: Setting {} to id: {}", name, rc.id());
	println!("{}", msg);
	lb.set_target(rc.id(), &msg)?;
	repo.set_head(&name)?;
	repo.checkout_head(Some(
		git2::build::CheckoutBuilder::default()
			// For some reason the force is required to make the working directory actually get updated
			// I suspect we should be adding some logic to handle dirty working directory states
			// but this is just an example so maybe not.
			.force(),
	))?;
	Ok(())
}

pub fn do_merge<'a>(
	repo: &'a git2::Repository,
	remote_branch: &str,
	fetch_commit: git2::AnnotatedCommit<'a>,
) -> Result<(), git2::Error> {
	// 1. do a merge analysis
	let analysis = repo.merge_analysis(&[&fetch_commit])?;

	if analysis.0.is_fast_forward() {
		println!("Doing a fast forward");
		// do a fast forward
		let refname = format!("refs/heads/{}", remote_branch);
		match repo.find_reference(&refname) {
			Ok(mut r) => {
				fast_forward(repo, &mut r, &fetch_commit)?;
			},
			Err(_) => {
				// The branch doesn't exist so just set the reference to the
				// commit directly. Usually this is because you are pulling
				// into an empty repository.
				repo.reference(
					&refname,
					fetch_commit.id(),
					true,
					&format!("Setting {} to {}", remote_branch, fetch_commit.id()),
				)?;
				repo.set_head(&refname)?;
				repo.checkout_head(Some(
					git2::build::CheckoutBuilder::default()
						.allow_conflicts(true)
						.conflict_style_merge(true)
						.force(),
				))?;
			},
		};
	} else {
		// println!("Nothing to do...");
	}
	Ok(())
}
