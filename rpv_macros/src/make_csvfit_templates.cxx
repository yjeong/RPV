#include <iostream>
#include <fstream>
#include <vector>

#include "TDirectory.h"
#include "TFile.h"

#include "utilities.hpp"
#include "utilities_macros.hpp"
#include "utilities_macros_rpv.hpp"

namespace {
  const int nBins=20;
  const float xMin=0.8484;
  const float xMax=1.0;

  TString lumi = "36.8";
  bool makeDatacard = true;
}

using namespace std;

vector<vector<TH1D*> > makeMCStatVariations(TString process, TH1D hist);
TH1D makeShapeHist(TString process, TString cut, TChain& chain);

int main(){ 
  TH1::SetDefaultSumw2(true);

  TString folder_dat = "/net/cms27/cms27r0/babymaker/babies/2017_01_27/data/merged_rpvdata_rpvregion/";  
  TString folder_bkg = "/net/cms27/cms27r0/babymaker/babies/2017_01_27/mc/merged_rpvmc_rpvregion/";
  TString folder_sig = "/net/cms2/cms2r0/jaehyeokyoo/babies/2017_01_10/mc/T1tbs/";

  vector<TString> s_data = getRPVProcess(folder_dat,"data");
  vector<TString> s_rpv_m1700 = getRPVProcess(folder_sig,"rpv_m1700");

  vector<TString> s_qcd = getRPVProcess(folder_bkg,"qcd");
  // The CSV reweighting only affects QCD; the flavor composition of other samples are well defined and does not need correction
  vector<TString> s_tt = getRPVProcess(folder_bkg,"ttbar");
  vector<TString> s_wjets = getRPVProcess(folder_bkg,"wjets");
  vector<TString> s_other = getRPVProcess(folder_bkg,"other_public");
  // Add ttbar and wjets to other
  s_other.insert(s_other.end(),s_tt.begin(),s_tt.end());
  s_other.insert(s_other.end(),s_wjets.begin(),s_wjets.end());

  //Make chains
  TChain data("tree"), rpv("tree"), qcd("tree"), other("tree");
  for(unsigned int ifile=0; ifile<s_data.size(); ifile++)      data.Add(s_data[ifile]);
  for(unsigned int ifile=0; ifile<s_rpv_m1700.size(); ifile++) rpv.Add(s_rpv_m1700[ifile]);
  for(unsigned int ifile=0; ifile<s_qcd.size(); ifile++)       qcd.Add(s_qcd[ifile]);
  for(unsigned int ifile=0; ifile<s_other.size(); ifile++)     other.Add(s_other[ifile]);
  
  TString basecut = "nleps==0&&ht>1500&&njets>=4&&njets<=7&&mj12>500&&nbm>=2&&pass";

  //Make shape hists
  TH1D h_data =   makeShapeHist("data_obs", basecut, data);
  TH1D h_rpv =    makeShapeHist("rpv", basecut, rpv);
  TH1D h_qcdb =   makeShapeHist("qcd_b", basecut, qcd);
  TH1D h_qcdc =   makeShapeHist("qcd_c", basecut, qcd);
  TH1D h_qcdl =   makeShapeHist("qcd_l", basecut, qcd);
  TH1D h_other =  makeShapeHist("other", basecut, other);

  cout<<"Normalizing QCD yields to data"<<endl;
  double scaling = (h_data.Integral() - h_other.Integral())/(h_qcdb.Integral() + h_qcdc.Integral() + h_qcdl.Integral());
  cout<<"Scaling factor is "<<scaling<<endl;

  h_qcdb.Scale(scaling);
  h_qcdc.Scale(scaling);
  h_qcdl.Scale(scaling);

  cout<<"Making MC stat variations"<<endl;
  vector<vector<TH1D*> > h_qcdb_mcstat =  makeMCStatVariations("qcd_b", h_qcdb);
  vector<vector<TH1D*> > h_qcdc_mcstat =  makeMCStatVariations("qcd_c", h_qcdc);
  vector<vector<TH1D*> > h_qcdl_mcstat =  makeMCStatVariations("qcd_l", h_qcdl);
  vector<vector<TH1D*> > h_other_mcstat = makeMCStatVariations("other", h_other);

  //Make file and write histograms to it
  TString filename = "csvfit_shapes.root";
  TFile *out = new TFile(filename, "recreate");
  TDirectory *bin =  out->mkdir("bin1");
  bin->cd();
  
  // Write out nominal shapes
  h_data.Write();
  h_rpv.Write();
  h_qcdb.Write();
  h_qcdc.Write();
  h_qcdl.Write();
  h_other.Write();

  // Write out mc stat. shapes
  for(unsigned int ibin=0; ibin<nBins; ibin++){
    for(unsigned int isys=0; isys<2; isys++){
      h_qcdb_mcstat[isys][ibin]->Write();
      h_qcdc_mcstat[isys][ibin]->Write();
      h_qcdl_mcstat[isys][ibin]->Write();
      h_other_mcstat[isys][ibin]->Write();
    }
  }

  out->Write();
  out->Close();

  cout<<"Wrote out shapes file: "<<filename<<endl;

  //Make datacard
  if(makeDatacard){
    double ndata = h_data.Integral();
    double nrpv = h_rpv.Integral();    
    double nqcdb = h_qcdb.Integral();
    double nqcdc = h_qcdc.Integral();
    double nqcdl =  h_qcdl.Integral();
    double nother = h_other.Integral();

    ofstream card;
    card.open("datacard_csvfit.dat");
    card << "# Datacard for csv flavor fit \n";
    card << "imax 1  number of channels \n";
    card << "jmax 4  number of backgrounds \n";
    card << "kmax *  number of nuisances \n";    
    card << "---------------------------------------------------------- \n";
    card << "shapes * bin1 csvfit_shapes.root bin1/$PROCESS bin1/$SYSTEMATIC \n";
    card << "---------------------------------------------------------- \n";
    card << "bin         bin1 \n";
    card << "observation "<<ndata<<" \n";
    card << "---------------------------------------------------------- \n";
    card << "bin         bin1      bin1     bin1     bin1     bin1 \n";
    card << "process     rpv    qcd_b    qcd_c    qcd_l    other \n";
    card << "process     0         1        2        3        4 \n";
    card << "rate        "<<nrpv<<"     "<<nqcdb<<"  "<<nqcdc<<"  "<<nqcdl<<"  "<<nother<<" \n";
    card << "---------------------------------------------------------- \n";
    card << "norm_qcd_b     lnU - 5 - - - \n";
    card << "norm_qcd_c     lnU - - 5 - - \n\n";
    
    for(unsigned int ibin=0; ibin<nBins; ibin++) card << "qcd_b_mcstat_bin"+to_string(ibin+1) << "  shape - 1 - - - \n";
    for(unsigned int ibin=0; ibin<nBins; ibin++) card << "qcd_c_mcstat_bin"+to_string(ibin+1) << "  shape - - 1 - - \n";
    for(unsigned int ibin=0; ibin<nBins; ibin++) card << "qcd_l_mcstat_bin"+to_string(ibin+1) << "  shape - - - 1 - \n";
    for(unsigned int ibin=0; ibin<nBins; ibin++) card << "other_mcstat_bin"+to_string(ibin+1) << "  shape - - - - 1 \n";

    cout<<"Made datacard: datacard_csvfit.dat"<<endl;
  }  
}

vector<vector<TH1D*> > makeMCStatVariations(TString process, TH1D hist){

  if(process != "qcd_b" && process != "qcd_c" && process != "qcd_l" && process != "other"){
    cout<<"ERROR: Incorrect process name"<<endl;
    exit(1);
  }

  vector<vector<TH1D*> > h_variations(2, vector<TH1D*>(nBins));

  for(unsigned int ibin=0; ibin<nBins; ibin++){
    for(unsigned int isys=0; isys<2; isys++){

      TString title = process+"_mcstat_bin"+to_string(ibin+1);
      title += isys==0 ? "Up" : "Down";

      h_variations[isys][ibin] = static_cast<TH1D*>(hist.Clone(title));
      double nomContent = h_variations[isys][ibin]->GetBinContent(ibin+1);
      double error = h_variations[isys][ibin]->GetBinError(ibin+1);
      double varContent = isys==0 ? nomContent+error : nomContent-error;

      h_variations[isys][ibin]->SetBinContent(ibin+1, varContent);
    }
  }
  return h_variations;
}

TH1D makeShapeHist(TString process, TString cut, TChain& chain){

  if(process!="data_obs" && process!="rpv" && process!="qcd_b" && process!="qcd_c" && process!="qcd_l" && process!="other"){
    cout<<"ERROR: Incorrect process name"<<endl;
    exit(1);
  }

  cout<<"Making "+process+" histogram"<<endl;

  if(process=="data_obs")    cut = cut;
  else if(process=="rpv")    cut = lumi+"*weight*("+cut+")";     
  else if(process=="qcd_b")  cut = lumi+"*weight*("+cut+"&&Sum$(jets_pt>30&&abs(jets_eta)<=2.4&&abs(jets_hflavor)==5)>=1)";
  else if(process=="qcd_c")  cut = lumi+"*weight*("+cut+"&&Sum$(jets_pt>30&&abs(jets_eta)<=2.4&&abs(jets_hflavor)==5)==0&&Sum$(jets_pt>30&&abs(jets_eta)<=2.4&&abs(jets_hflavor)==4)>=1)";
  else if(process=="qcd_l")  cut = lumi+"*weight*("+cut+"&&Sum$(jets_pt>30&&abs(jets_eta)<=2.4&&abs(jets_hflavor)==5)==0&&Sum$(jets_pt>30&&abs(jets_eta)<=2.4&&abs(jets_hflavor)==4)==0)";
  else if(process=="other")  cut = lumi+"*weight*("+cut+"&&stitch_ht)";     

  TH1D h_temp(process, process, nBins, xMin, xMax);
  chain.Project(process,"jets_csv",cut);

  return h_temp;
}