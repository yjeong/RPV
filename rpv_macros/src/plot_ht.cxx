#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <stdlib.h>
#include <stdio.h>
#include <iomanip>

#include "TCanvas.h"
#include "TH1F.h"
#include "TFile.h"
#include "TStyle.h"
#include "TLegend.h"
#include "TDirectory.h"
#include "TString.h"
#include "TTree.h"
#include "THStack.h"
using namespace std;

void set_histo_style(TH1F *h){
	h->GetXaxis()->SetTitle("HT");
	h->SetLineWidth(1.2);
}

void set_legend_style(TLegend *l1){
	l1->SetFillColor(0);
	l1->SetLineColor(0);
	l1->SetLineStyle(kSolid);
	l1->SetLineWidth(1);
	l1->SetFillStyle(1001);
	l1->SetTextFont(42);
	l1->SetTextSize(0.04);
}

int main(){
	float ht;
	float w_isr_tr, w_lumi, weight;
	const int nfiles = 4;
	TString inputdir, outputdir;
	outputdir = "/cms/scratch/yjeong/CMSSW_10_2_13/src/RPV/rpv_macros/plots_ht/";
	inputdir = "/xrootd_user/yjeong/xrootd/nanoprocessing/2016/merged_rpvfitnbge0/";

	TString filename[nfiles] = {
		"600to800_TuneCUETP8M1_18",
		"800to1200_TuneCUETP8M1_14",
		"1200to2500_TuneCUETP8M1_7",
		"2500toInf_TuneCUETP8M1_4"
	};
	//---------------TTJets_HT_All-------------------
	TString alltt;
	alltt = "TTJets_TuneCUETP8M1_11";
	TFile *f1;
	TTree *t1;
	TH1F *h2;
	TCanvas *c1;
	c1 = new TCanvas;
	c1->SetLogy();
	f1 = new TFile(inputdir+alltt+".root");
	t1 = (TTree*)f1->Get("tree");
	h2 = new TH1F("h2","TTJets_HT",100,1100,7000);
	for(int i = 0; i<t1->GetEntries(); i++){
		t1->GetEntry(i);
		t1->SetBranchAddress("ht",&ht);
		t1->SetBranchAddress("w_isr_tr",&w_isr_tr);
		t1->SetBranchAddress("w_lumi",&w_lumi);
		t1->SetBranchAddress("weight",&weight);
		h2->Fill(ht,weight);
	}
	h2->Draw("hist");
	c1->SaveAs(outputdir+"TTJets_All_Ht.png");
	//------------------------------------------------

	TFile *tfile[nfiles];
	TTree *tree[nfiles];
	TH1F *h1[nfiles];
	TCanvas *c2;
	c2 = new TCanvas;
	c2->SetLogy();
	TCanvas *c3;
	c3 = new TCanvas;
	c3->SetLogy();
	TLegend *l;
	THStack *hs;
	l = new TLegend(0.65,0.54,0.75,0.8);
	set_legend_style(l);
	float ymax = 0;
	hs = new THStack("hs","TTJets_HT");
	for(int i = 0; i < nfiles; i++){
		tfile[i] = new TFile(inputdir+"TTJets_HT-"+filename[i]+".root");
		tree[i] = (TTree*)tfile[i]->Get("tree");
		tree[i]->SetBranchAddress("ht",&ht);
		tree[i]->SetBranchAddress("w_isr_tr",&w_isr_tr);
		tree[i]->SetBranchAddress("w_lumi",&w_lumi);
		tree[i]->SetBranchAddress("weight",&weight);

		h1[i] = new TH1F(Form("h1_%d",i),Form("TTJets_HT"),100,1100,7000);
		set_histo_style(h1[i]);
		for(int j=0; j<tree[i]->GetEntries();j++){
			tree[i]->GetEntry(j);
			h1[i]->Fill(ht,weight);
		}

		cout<<"weight: "<<weight<<", w_isr: "<<w_isr_tr<<", w_lumi: "<<w_lumi<<endl;
		if(i==0){
			h1[i]->SetLineColor(kRed);
			l->AddEntry(h1[i],"600to800");
		}
		else if(i==1){
			h1[i]->SetLineColor(kBlue);
			l->AddEntry(h1[i],"800to1200");
		}
		else if(i==2){
			h1[i]->SetLineColor(kGreen);
			l->AddEntry(h1[i],"1200to2500");
		}
		else if(i==3){
			h1[i]->SetLineColor(kOrange+1);
			l->AddEntry(h1[i],"2500toInf");
		}
		hs->Add(h1[i]);
		c2->cd();
		if(i==0){
			ymax = h1[i]->GetMaximum();
			h1[i]->SetMaximum(ymax*1.5);
			h1[i]->Draw("hist");
		}
		else if(i>0)h1[i]->Draw("histsame");

	}
	l->Draw();
	c2->SaveAs(outputdir+"TTJets_Ht.png");
	c3->cd();
	hs->SetMaximum(ymax*1.5);
	hs->Draw("hist");
	h2->Draw("histsame");
	l->Draw();
	c3->SaveAs(outputdir+"TTJets_Ht_stack.png");
}
